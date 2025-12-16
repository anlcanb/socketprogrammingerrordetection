#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSIZE 4096

//Yardımcı: Bit sayma
int count_ones_in_byte(unsigned char c) {
    int ones = 0;
    for (int i = 0; i < 8; i++) {
        if ((c >> i) & 1) ones++;
    }
    return ones;
}

//  Parity (Even)
void compute_parity(const char *data, char *out, size_t out_size) {
    int ones = 0;
    for (int i = 0; data[i] != '\0'; i++) {
        ones += count_ones_in_byte((unsigned char)data[i]);
    }
    if (out_size > 0) {
        out[0] = (ones % 2 == 0) ? '0' : '1';
        if (out_size > 1) out[1] = '\0';
    }
}

// 2D Parity (8x8)
void compute_parity2d_8x8(const char *data, char *out, size_t out_size) {
    unsigned char matrix[8][8];
    unsigned char padded[64];
    size_t len = strlen(data);

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
    for (int r = 0; r < 8; r++) {
        int ones = 0;
        for (int c = 0; c < 8; c++) {
            ones += count_ones_in_byte(matrix[r][c]);
        }
        if (ones % 2 != 0) {
            bits |= (1u << (15 - r));
        }
    }
    for (int c = 0; c < 8; c++) {
        int ones = 0;
        for (int r = 0; r < 8; r++) {
            ones += count_ones_in_byte(matrix[r][c]);
        }
        if (ones % 2 != 0) {
            bits |= (1u << (7 - c));
        }
    }

    if (out_size >= 5) {
        snprintf(out, out_size, "%04X", bits);
    } else if (out_size > 0) {
        out[0] = '\0';
    }
}

// CRC16-CCITT
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

// Internet Checksum
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
        if (sum & 0x10000) {
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

// Hamming (12,8 per char)
void compute_hamming_simple(const char *data, char *out, size_t out_size) {
    char *p = out;
    size_t remaining = out_size;

    for (size_t i = 0; data[i] != '\0'; i++) {
        unsigned char d = (unsigned char)data[i];

        int bit[13] = {0};
        bit[3]  = (d >> 7) & 1;
        bit[5]  = (d >> 6) & 1;
        bit[6]  = (d >> 5) & 1;
        bit[7]  = (d >> 4) & 1;
        bit[9]  = (d >> 3) & 1;
        bit[10] = (d >> 2) & 1;
        bit[11] = (d >> 1) & 1;
        bit[12] = (d >> 0) & 1;

        bit[1] = bit[3] ^ bit[5] ^ bit[7] ^ bit[9] ^ bit[11];
        bit[2] = bit[3] ^ bit[6] ^ bit[7] ^ bit[10] ^ bit[11];
        bit[4] = bit[5] ^ bit[6] ^ bit[7] ^ bit[12];
        bit[8] = bit[9] ^ bit[10] ^ bit[11] ^ bit[12];

        uint16_t encoded = 0;
        for (int b = 1; b <= 12; b++) {
            encoded <<= 1;
            encoded |= (bit[b] & 1);
        }

        if (remaining <= 4) break;
        int written = snprintf(p, remaining, "%03X", encoded);
        p += written;
        remaining -= written;
    }

    if (remaining > 0) *p = '\0';
    else if (out_size > 0) out[out_size - 1] = '\0';
}

//Metoda göre kontrol seç
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
    int s_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(s_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6000);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s_listen, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s_listen);
        return 1;
    }

    if (listen(s_listen, 1) < 0) {
        perror("listen");
        close(s_listen);
        return 1;
    }

    printf("Client2: Server1 için 6000 portunda dinlemede...\n");

    int conn = accept(s_listen, NULL, NULL);
    if (conn < 0) {
        perror("accept");
        close(s_listen);
        return 1;
    }

    char buffer[BUFSIZE];
    int n = recv(conn, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        perror("recv");
        close(conn);
        close(s_listen);
        return 1;
    }
    buffer[n] = '\0';

    char data[BUFSIZE], method[64], incoming_control[BUFSIZE];
    if (sscanf(buffer, "%[^|]|%[^|]|%s", data, method, incoming_control) != 3) {
        fprintf(stderr, "Paket biçimi hatalı.\n");
        close(conn);
        close(s_listen);
        return 1;
    }

    char computed[BUFSIZE];
    compute_control_by_method(method, data, computed, sizeof(computed));

    int ok = (strcmp(computed, incoming_control) == 0);

    printf("\n RECEIVED PACKET \n");
    printf("Received Data       : %s\n", data);
    printf("Method              : %s\n", method);
    printf("Sent Check Bits     : %s\n", incoming_control);
    printf("Computed Check Bits : %s\n", computed);
    printf("Status              : %s\n",
           ok ? "DATA CORRECT" : "DATA CORRUPTED");
    printf("   \n\n");

    close(conn);
    close(s_listen);
    return 0;
}
