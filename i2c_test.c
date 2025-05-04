
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <string.h>
#include <linux/i2c-dev.h>

#define SUCCESS 2
#define FAILURE 12

#define ADDR 0x14
#define I2C_BUS "/dev/i2c-2"



#pragma pack(push,1)  // start 1-byte packing

typedef struct {
    int16_t left_speed;
    int16_t right_speed;
    int16_t left_dist;
    int16_t right_dist;
    bool    r_led;
    bool    g_led;
    bool    y_led;
} I2C_Command_Packet;  // sizeof == 11

typedef struct {
    int16_t  l_enc;
    int16_t  r_enc;
    int16_t  rem_left;
    int16_t  rem_right;
    int16_t  cmd_left_dist;
    int16_t  cmd_right_dist;
    int16_t  cmd_left_speed;
    int16_t  cmd_right_speed;
    int16_t  set_left_speed;
    int16_t  set_right_speed;
    uint16_t batteryMillivolts;
    bool     button_A;
    bool     button_B;
    bool     button_C;
} I2C_Telem_Packet; // sizeof == 25

typedef struct {
    I2C_Command_Packet cmd;    // 11 bytes
    I2C_Telem_Packet telem;  // 25 bytes
} I2C_Data;            // sizeof == 36

#define PACKET_SIZE 36
#define TELEMETRY_START 11
#define PACKET_START 0

#pragma pack(pop)   // end packing

// Sanity checks (requires )
_Static_assert(sizeof(I2C_Command_Packet)  == 11, "Commands must be 11 bytes");
_Static_assert(sizeof(I2C_Telem_Packet) == 25, "Telemetry must be 25 bytes");
_Static_assert(sizeof(I2C_Data)      == 36, "Data must be 36 bytes");


typedef struct {
    int fd;
} I2C_Handle;

int open_i2c(int addr) {
   int fd = open(I2C_BUS, O_RDWR);
  if (fd < 0) {
      perror("Opening I2C bus");
      return -1;
  }
  if (ioctl(fd, I2C_SLAVE, addr) < 0) {
      perror("Selecting I2C device");
      close(fd);
      return -1;
  }
  return fd;
}

int close_i2c(I2C_Handle *h) {
    if(!h) {
        return 0;
    }
    close(h->fd);
    free(h);

    return 1;
}

void i2c_send(int fd, I2C_Command_Packet* packet) {
  uint8_t buffer[12] = {0};
  memcpy(buffer + 1, packet, TELEMETRY_START);
  if (write(fd, buffer, 12) != TELEMETRY_START + 1) {
      perror("I2C write of command");
      close(fd);
      return;
  }
  fsync(fd);
  sleep(1);
}

// #pragma pack(push,1)
// typedef struct {
//     I2C_Command_Packet cmd;   // 11 bytes
//     I2C_Telem_Packet   telem; // 25 bytes
// } I2C_Data;                      // 36 bytes total
// #pragma pack(pop)

// Revised i2c_read:
int i2c_read(int fd, I2C_Data* packet) {
    I2C_Data buf;
    
    uint8_t wbuf[1] = {0};
    if (write(fd, wbuf, 1) != 1)
    {
        return FAILURE;
    }

    usleep(500);
    ssize_t nr = read(fd, &buf, sizeof(I2C_Data));
    if (nr != sizeof(buf)) {
        perror("I2C read of Data");
        close(fd);
        return FAILURE;
    }
    memcpy(packet, &buf, PACKET_SIZE);

    printf("Raw Data: \n");
    uint8_t* bytes = (uint8_t*)&buf;
    for (size_t i = 0; i < sizeof(buf); i++) {
        printf("Byte %d: %d\n", i, bytes[i]);
    }
    printf("\n");

    return SUCCESS;
}

void print_telemetry(I2C_Telem_Packet *telem) {
  printf("\n=== TELEMETRY DATA ===\n");
  printf("Encoders: Left = %d, Right = %d\n", telem->l_enc, telem->r_enc);
  printf("Remaining distance: Left = %d, Right = %d\n", telem->rem_left, telem->rem_right);
  printf("Commanded distance: Left = %d, Right = %d\n", telem->cmd_left_dist, telem->cmd_right_dist);
  printf("Commanded speed: Left = %d, Right = %d\n", telem->cmd_left_speed, telem->cmd_right_speed);
  printf("Set speed: Left = %d, Right = %d\n", telem->set_left_speed, telem->set_right_speed);
  printf("Battery: %.2f V\n", telem->batteryMillivolts / 1000.0);
  printf("Buttons: A = %s, B = %s, C = %s\n", 
         telem->button_A ? "Pressed" : "Not pressed",
         telem->button_B ? "Pressed" : "Not pressed", 
         telem->button_C ? "Pressed" : "Not pressed");
  printf("=======================\n");
}


int stop_robot(int fd) {
    if (fd < 0) return FAILURE;
    I2C_Command_Packet packet = {0};
    i2c_send(fd, &packet);

    return SUCCESS;
}

int spin(int fd, int left) {
    
    if (fd < 0) return FAILURE;
    I2C_Command_Packet packet = {0};
    packet.right_speed = left * 100;
    packet.left_speed = left * (-1) * 100;
    i2c_send(fd, &packet);

    return SUCCESS;
}

int forward(int fd, int dist) {
    if (fd < 0) return FAILURE;
    I2C_Command_Packet packet = {0};
    packet.right_dist = dist;
    packet.left_dist = dist;
    i2c_send(fd, &packet);

    return SUCCESS;
}


int main() {
  int fd = open_i2c(ADDR);
  if (fd < 0) return 1;
  printf("Fd: %d\n", fd);
  
  forward(fd, 100);
  sleep(3);
  spin(fd, 1);
  sleep(3);
  stop_robot(fd);
  

  I2C_Data received_packet = {0};
  int success = i2c_read(fd, &received_packet);
  if(success == SUCCESS) {
    print_telemetry(&received_packet.telem);
  }
 
//close_i2c(fd);


    // uint8_t write_buf[12] = {0};
    // write_buf[1] = 0x00;
    // write_buf[3] = 0x00;

    // if (write(fd, write_buf, 12) != 12) {
    // perror("Broken Send");
    // return 1;
    // }
    // fsync(fd);
    // sleep(1);
    // uint8_t read_buf[37] = {0};
    // if (read(fd, &read_buf, 37) != 37) {
    //     perror("Broken Read");
    //     return 1;
    // }
    // for (size_t i = 0; i < 37; i++) {
    //     printf("Byte %d: 0x%02X \n", i, read_buf[i]);
    // }
return 0;
}