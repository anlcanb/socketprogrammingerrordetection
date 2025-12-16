#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSIZE 4096

//Hata enjesdafksiyon metodları

//  Bit flip
void error_bit_flip(char *data) {
    int len = strlen(data);
    if (len == 0) return;
    int pos = rand() % len;
    int bit_pos = rand() % 8;
    data[pos] ^= (1 << bit_pos);
}

// Character substitution
void error_char_substitution(char *data) {
    int len = strlen(data);
    if (len == 0) return;
    int pos = rand() % len;
    char new_char = 'A' + (rand() % 26);
    data[pos] = new_char;
}

// Character deletion
void error_char_deletion(char *data) {
    int len = strlen(data);
    if (len <= 1) return;
    int pos = rand() % len;
    for (int i = pos; i < len; i++) {
        data[i] = data[i + 1];
    }
}

// Random character insertion
void error_char_insertion(char *data) {
    int len = strlen(data);
    if (len >= BUFSIZE - 2) return;
    int pos = rand() % (len + 1);
    char new_char = 'a' + (rand() % 26);
    for (int i = len; i >= pos; i--) {
        data[i + 1] = data[i];
    }
    data[pos] = new_char;
}

//Character swapping
void error_char_swapping(char *data) {
    int len = strlen(data);
    if (len <= 1) return;
    int pos = rand() % (len - 1);
    char tmp = data[pos];
    data[pos] = data[pos + 1];
    data[pos + 1] = tmp;
}

//Multiple bit flips
void error_multiple_bit_flips(char *data) {
    int flips = 2 + rand() % 5; // 2-6 arası random
    for (int i = 0; i < flips; i++) {
        error_bit_flip(data);
    }
}

// Burst error
void error_burst(char *data) {
    int len = strlen(data);
    if (len <= 3) return;
    int burst_len = 3 + rand() % 6; // 3-8
    if (burst_len > len) burst_len = len;
    int start = rand() % (len - burst_len + 1);
    for (int i = start; i < start + burst_len; i++) {
        char new_char = '0' + (rand() % 10);
        data[i] = new_char;
    }
}

void apply_random_error(char *data, int *error_type_used) {
    int choice = 1 + rand() % 7; // 1..7
    *error_type_used = choice;
    switch (choice) {
        case 1: error_bit_flip(data); break;
        case 2: error_char_substitution(data); break;
        case 3: error_char_deletion(data); break;
        case 4: error_char_insertion(data); break;
        case 5: error_char_swapping(data); break;
        case 6: error_multiple_bit_flips(data); break;
        case 7: error_burst(data); break;
    }
}

const char* error_type_name(int t) {
    switch (t) {
        case 1: return "Bit Flip";
        case 2: return "Character Substitution";
        case 3: return "Character Deletion";
        case 4: return "Character Insertion";
        case 5: return "Character Swapping";
        case 6: return "Multiple Bit Flips";
        case 7: return "Burst Error";
        default: return "None";
    }
}

int main() {
    srand((unsigned)time(NULL));

    // Client1 için dinle (port 5000)
    int s_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(s_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr1;
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(5000);
    addr1.sin_addr.s_addr = INADDR_ANY;

    if (bind(s_listen, (struct sockaddr*)&addr1, sizeof(addr1)) < 0) {
        perror("bind");
        close(s_listen);
        return 1;
    }

    if (listen(s_listen, 1) < 0) {
        perror("listen");
        close(s_listen);
        return 1;
    }

    printf("Server1: Client1 için 5000 portunda dinlemede...\n");
    int client1 = accept(s_listen, NULL, NULL);
    if (client1 < 0) {
        perror("accept");
        close(s_listen);
        return 1;
    }

    char buffer[BUFSIZE];
    int n = recv(client1, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        perror("recv");
        close(client1);
        close(s_listen);
        return 1;
    }
    buffer[n] = '\0';

    printf("Server1: Client1'den gelen paket:\n%s\n", buffer);

    // Paket ayırma DATA|METHOD|CONTROL
    char data[BUFSIZE], method[64], control[BUFSIZE];
    if (sscanf(buffer, "%[^|]|%[^|]|%s", data, method, control) != 3) {
        fprintf(stderr, "Paket biçimi hatalı.\n");
        close(client1);
        close(s_listen);
        return 1;
    }

    printf("Orijinal Data: %s\n", data);

    int error_type_used = 0;
    apply_random_error(data, &error_type_used);

    printf("Uygulanan Hata Türü: %s\n", error_type_name(error_type_used));
    printf("Bozulmuş Data: %s\n", data);

    char new_packet[BUFSIZE];
    snprintf(new_packet, sizeof(new_packet), "%s|%s|%s", data, method, control);

    //Client2'ye bağlan (port 6000)
    int s_to_client2 = socket(AF_INET, SOCK_STREAM, 0);
    if (s_to_client2 < 0) {
        perror("socket client2");
        close(client1);
        close(s_listen);
        return 1;
    }

    struct sockaddr_in addr2;
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(6000);
    addr2.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Server1: Client2'ye bağlanmaya çalışılıyor (6000)...\n");
    if (connect(s_to_client2, (struct sockaddr*)&addr2, sizeof(addr2)) < 0) {
        perror("connect client2");
        close(s_to_client2);
        close(client1);
        close(s_listen);
        return 1;
    }

    if (send(s_to_client2, new_packet, strlen(new_packet), 0) < 0) {
        perror("send to client2");
        close(s_to_client2);
        close(client1);
        close(s_listen);
        return 1;
    }

    printf("Server1: Bozulmuş paket Client2'ye gönderildi.\n");

    close(s_to_client2);
    close(client1);
    close(s_listen);
    return 0;
}
