#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BMP_HEADER_SIZE 54
#define JPG_HEADER_SIZE 20
#define PNG_HEADER_SIZE 8
#define DEFAULT_OFFSET 1024

// Função para identificar o tipo de arquivo
const char* identifyFileType(const unsigned char *fileData) {
    if (fileData[0] == 0x42 && fileData[1] == 0x4D) {
        return "BMP";
    } else if (fileData[0] == 0xFF && fileData[1] == 0xD8) {
        return "JPG";
    } else if (fileData[0] == 0x89 && fileData[1] == 0x50 &&
               fileData[2] == 0x4E && fileData[3] == 0x47 &&
               fileData[4] == 0x0D && fileData[5] == 0x0A &&
               fileData[6] == 0x1A && fileData[7] == 0x0A) {
        return "PNG";
    } else {
        return "UNKNOWN";
    }
}

// Função para calcular o offset adequado com base no tipo de arquivo
size_t calculateHeaderSize(const char* fileType) {
    if (strcmp(fileType, "BMP") == 0) {
        return BMP_HEADER_SIZE;
    } else if (strcmp(fileType, "JPG") == 0) {
        return DEFAULT_OFFSET; // Usamos DEFAULT_OFFSET para JPG para garantir espaço seguro
    } else if (strcmp(fileType, "PNG") == 0) {
        return PNG_HEADER_SIZE;
    } else {
        return DEFAULT_OFFSET;
    }
}

// Função para esconder uma mensagem em um arquivo genérico
void hideMessageInFile(const char *inputFilePath, const char *outputFilePath, const char *message) {
    printf("Opening input file: %s\n", inputFilePath);
    FILE *inputFile = fopen(inputFilePath, "rb");
    if (inputFile == NULL) {
        perror("Error opening input file");
        return;
    }

    printf("Opening output file: %s\n", outputFilePath);
    FILE *outputFile = fopen(outputFilePath, "wb");
    if (outputFile == NULL) {
        perror("Error opening output file");
        fclose(inputFile);
        return;
    }

    fseek(inputFile, 0, SEEK_END);
    long inputFileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    printf("Input file size: %ld bytes\n", inputFileSize);

    unsigned char *fileData = (unsigned char *)malloc(inputFileSize);
    if (fileData == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(inputFile);
        fclose(outputFile);
        return;
    }

    size_t bytesRead = fread(fileData, 1, inputFileSize, inputFile);
    if (bytesRead != inputFileSize) {
        fprintf(stderr, "Error reading input file\n");
        free(fileData);
        fclose(inputFile);
        fclose(outputFile);
        return;
    }
    fclose(inputFile);

    const char *fileType = identifyFileType(fileData);
    size_t headerSize = calculateHeaderSize(fileType);
    printf("Identified file type: %s, header size: %zu bytes\n", fileType, headerSize);

    size_t messageLength = strlen(message);
    size_t maxMessageLength = (inputFileSize - headerSize) / 8; // Calculando o espaço disponível para a mensagem
    printf("Maximum message length: %zu bytes\n", maxMessageLength);
    if (messageLength > maxMessageLength) {
        free(fileData);
        fprintf(stderr, "Message too long to hide in file. Maximum length: %zu bytes\n", maxMessageLength);
        return;
    }

    // Esconder a mensagem nos bits menos significativos dos bytes de dados de pixel
    for (size_t i = 0; i < messageLength; ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            size_t byteIndex = headerSize + (i * 8 + bit);
            if (byteIndex < inputFileSize) {
                fileData[byteIndex] = (fileData[byteIndex] & 0xFE) | ((message[i] >> bit) & 1);
            }
        }
    }

    size_t bytesWritten = fwrite(fileData, 1, inputFileSize, outputFile);
    if (bytesWritten != inputFileSize) {
        fprintf(stderr, "Error writing output file\n");
        free(fileData);
        fclose(outputFile);
        return;
    }
    fclose(outputFile);

    free(fileData);

    printf("Message hidden successfully in %s\n", outputFilePath);
}

// Função para encontrar uma mensagem escondida em um arquivo genérico
void findMessageInFile(const char *filePath, size_t messageLength) {
    printf("Opening file: %s\n", filePath);
    FILE *file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("File size: %ld bytes\n", fileSize);

    unsigned char *fileData = (unsigned char *)malloc(fileSize);
    if (fileData == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return;
    }

    size_t bytesRead = fread(fileData, 1, fileSize, file);
    if (bytesRead != fileSize) {
        fprintf(stderr, "Error reading file\n");
        free(fileData);
        fclose(file);
        return;
    }
    fclose(file);

    const char *fileType = identifyFileType(fileData);
    size_t headerSize = calculateHeaderSize(fileType);
    printf("Identified file type: %s, header size: %zu bytes\n", fileType, headerSize);

    char *message = (char *)malloc(messageLength + 1);
    if (message == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(fileData);
        return;
    }

    // Encontrar a mensagem nos bits menos significativos dos bytes de dados de pixel
    for (size_t i = 0; i < messageLength; ++i) {
        message[i] = 0;
        for (int bit = 0; bit < 8; ++bit) {
            size_t byteIndex = headerSize + (i * 8 + bit);
            if (byteIndex < fileSize) {
                message[i] |= (fileData[byteIndex] & 1) << bit;
            }
        }
    }
    message[messageLength] = '\0';

    printf("Hidden message: %s\n", message);

    free(fileData);
    free(message);
}

int main() {
    char inputFilePath[256];
    char outputFilePath[256];
    char choice;
    char message[1024];

    printf("Enter the file path: ");
    scanf("%255s", inputFilePath);

    printf("Do you want to (h)ide a message or (f)ind a message? ");
    scanf(" %c", &choice);

    if (choice == 'h') {
        printf("Enter the output file path: ");
        scanf("%255s", outputFilePath);
        printf("Enter the message to hide: ");
        scanf(" %[^\n]", message); // Ler a mensagem, incluindo espaços

        hideMessageInFile(inputFilePath, outputFilePath, message);
    } else if (choice == 'f') {
        size_t messageLength;
        printf("Enter the length of the hidden message: ");
        scanf("%zu", &messageLength);

        findMessageInFile(inputFilePath, messageLength);
    } else {
        printf("Invalid choice\n");
    }

    return 0;
}