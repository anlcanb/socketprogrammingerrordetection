#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSIZE 4096

//Yardımcı bit sayma
int count_ones_in_byte(unsigned char c) {
    int ones = 0;
    for (int i = 0; i < 8; i++) {
        if ((c >> i) & 1) ones++;
    }
    return ones;
}

//Parity
void compute_parity(const char *data, char *out, size_t out_size) {
    int ones = 0;
    for (int i = 0; data[i] != '\0'; i++) {
        ones += count_ones_in_byte((unsigned char)data[i]);
    }
    //toplam 1 sayısı çift olmalı kontrol biti .
    if (out_size > 0) {
        out[0] = (ones % 2 == 0) ? '0' : '1';
        if (out_size > 1) out[1] = '\0';
    }
}

//2D Parity(8x8)

void compute_parity2d_8x8(const char *data, char *out, size_t out_size) {
    unsigned char matrix[8][8];
    unsigned char padded[64];
    size_t len = strlen(data);

    // 64 karaktere kadar doldurulur, yetmezse space ile padding yapılır
    for (int i = 0; i < 64; i++) {
        if ((size_t)i < len) padded[i] = (unsigned char)data[i];
        else padded[i] = ' ';
    }

    int idx = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            matrix[r][c] = padded[idx++];
        }
    }

    uint16_t bits = 0;
    // İlk 8 bit satır parity, sonraki 8 bit sütun parity
    for (int r = 0; r < 8; r++) {
        int ones = 0;
        for (int c = 0; c < 8; c++) {
            ones += count_ones_in_byte(matrix[r][c]);
        }
        if (ones % 2 != 0) {
            bits |= (1u << (15 - r)); // üst 8 bit
        }
    }
    for (int c = 0; c < 8; c++) {
        int ones = 0;
        for (int r = 0; r < 8; r++) {
            ones += count_ones_in_byte(matrix[r][c]);
        }
        if (ones % 2 != 0) {
            bits |= (1u << (7 - c)); // alt 8 bit
        }
    }

    if (out_size >= 5) {
        snprintf(out, out_size, "%04X", bits);
    } else if (out_size > 0) {
        out[0] = '\0';
    }
}

//CRC-16 (CCITT, poly=0x1021)
uint16_t crc16_ccitt(const unsigned char *data) {
    uint16_t crc = 0xFFFF;
    while (*data) {
        crc ^= ((uint16_t)(*data)) << 8;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
        data++;
    }
    return crc;
}

void compute_crc16(const char *data, char *out, size_t out_size) {
    uint16_t crc = crc16_ccitt((const unsigned char*)data);
    if (out_size >= 5) {
        snprintf(out, out_size, "%04X", crc & 0xFFFF);
    } else if (out_size > 0) {
        out[0] = '\0';
    }
}

//Internet Checksum (IP checksum)
void compute_internet_checksum(const char *data, char *out, size_t out_size) {
    uint32_t sum = 0;
    const unsigned char *ptr = (const unsigned char *)data;

    while (*ptr) {
        uint16_t word = *ptr << 8;
        ptr++;
        if (*ptr) {
            word |= *ptr;
            ptr++;
        }
        sum += word;
        if (sum & 0x10000) {  // carry
            sum = (sum & 0xFFFF) + 1;
        }
    }

    uint16_t checksum = ~(sum & 0xFFFF);
    if (out_size >= 5) {
        snprintf(out, out_size, "%04X", checksum);
    } else if (out_size > 0) {
        out[0] = '\0';
    }
}

//Hamming Code (basit Hamming(12,8) her karaktere)

void compute_hamming_simple(const char *data, char *out, size_t out_size) {
    char *p = out;
    size_t remaining = out_size;

    for (size_t i = 0; data[i] != '\0'; i++) {
        unsigned char d = (unsigned char)data[i];

        int bit[13] = {0}; // 1..12 kullan
        bit[3]  = (d >> 7) & 1; // d1
        bit[5]  = (d >> 6) & 1; // d2
        bit[6]  = (d >> 5) & 1; // d3
        bit[7]  = (d >> 4) & 1; // d4
        bit[9]  = (d >> 3) & 1; // d5
        bit[10] = (d >> 2) & 1; // d6
        bit[11] = (d >> 1) & 1; // d7
        bit[12] = (d >> 0) & 1; // d8

        bit[1] = bit[3] ^ bit[5] ^ bit[7] ^ bit[9] ^ bit[11];
        bit[2] = bit[3] ^ bit[6] ^ bit[7] ^ bit[10] ^ bit[11];
        bit[4] = bit[5] ^ bit[6] ^ bit[7] ^ bit[12];
        bit[8] = bit[9] ^ bit[10] ^ bit[11] ^ bit[12];

        uint16_t encoded = 0;
        for (int b = 1; b <= 12; b++) {
            encoded <<= 1;
            encoded |= (bit[b] & 1);
        }

        if (remaining <= 4) break; // yer kalmazsa
        int written = snprintf(p, remaining, "%03X", encoded);
        p += written;
        remaining -= written;
    }

    if (remaining > 0) *p = '\0';
    else if (out_size > 0) out[out_size - 1] = '\0';
}

// Metoda göre kontrol seçimi
void compute_control_by_method(const char *method,
                               const char *data,
                               char *out,
                               size_t out_size) {
    if (strcmp(method, "PARITY") == 0) {
        compute_parity(data, out, out_size);
    } else if (strcmp(method, "PARITY2D") == 0) {
        compute_parity2d_8x8(data, out, out_size);
    } else if (strcmp(method, "CRC16") == 0) {
        compute_crc16(data, out, out_size);
    } else if (strcmp(method, "CHECKSUM") == 0) {
        compute_internet_checksum(data, out, out_size);
    } else if (strcmp(method, "HAMMING") == 0) {
        compute_hamming_simple(data, out, out_size);
    } else {
        if (out_size > 0) out[0] = '\0';
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);              // server1 dinliyor
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    char data[BUFSIZE];
    printf("Gönderilecek metni girin ( '|' kullanmayın ): ");
    if (!fgets(data, sizeof(data), stdin)) {
        fprintf(stderr, "Girdi okunamadı.\n");
        close(sock);
        return 1;
    }
    data[strcspn(data, "\n")] = '\0';

    printf("\nHata tespit yöntemi seçin:\n");
    printf("1) PARITY (Even)\n");
    printf("2) 2D PARITY (8x8)\n");
    printf("3) CRC16\n");
    printf("4) HAMMING\n");
    printf("5) CHECKSUM (Internet)\n");
    printf("Seçim (1-5): ");

    int choice;
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Geçersiz seçim.\n");
        close(sock);
        return 1;
    }

    char method[16];
    switch (choice) {
        case 1: strcpy(method, "PARITY"); break;
        case 2: strcpy(method, "PARITY2D"); break;
        case 3: strcpy(method, "CRC16"); break;
        case 4: strcpy(method, "HAMMING"); break;
        case 5: strcpy(method, "CHECKSUM"); break;
        default:
            printf("Geçersiz seçim, CRC16 kullanılıyor.\n");
            strcpy(method, "CRC16");
    }

    char control[BUFSIZE];
    compute_control_by_method(method, data, control, sizeof(control));

    char packet[BUFSIZE];
    snprintf(packet, sizeof(packet), "%s|%s|%s", data, method, control);

    printf("\nGönderilen Paket:\n%s\n\n", packet);

    if (send(sock, packet, strlen(packet), 0) < 0) {
        perror("send");
        close(sock);
        return 1;
    }

    printf("Paket server1'e gönderildi.\n");

    close(sock);
    return 0;
}
