
/*
 * Task 29 - ESP32 UART ACK Firmware
 *
 * Purpose:
 * 1. Receive an 8-byte packet from Windows C.
 * 2. Detect packet header AA 55.
 * 3. Validate XOR checksum.
 * 4. Send ACK (0x06) for valid packets.
 *
 * UART Configuration:
 * Baud Rate : 115200
 * Data Bits : 8
 * Parity    : None
 * Stop Bits : 1
 */

#define PACKET_SIZE 8

#define HEADER_1 0xAA
#define HEADER_2 0x55

#define ACK_BYTE 0x06

uint8_t packet[PACKET_SIZE];
size_t packetCount = 0;


// Calculate XOR checksum
uint8_t calculateChecksum(const uint8_t *data)
{
    uint8_t checksum = 0;

    for (int i = 0; i < PACKET_SIZE - 1; i++)
    {
        checksum ^= data[i];
    }

    return checksum;
}


// Process incoming UART bytes
void processIncomingByte(uint8_t incomingByte)
{
    // Add received byte to packet buffer
    packet[packetCount++] = incomingByte;

    // Search for packet header AA 55
    while (packetCount >= 2 &&
           (packet[0] != HEADER_1 || packet[1] != HEADER_2))
    {
        // Discard one byte and shift remaining bytes
        for (size_t i = 1; i < packetCount; i++)
        {
            packet[i - 1] = packet[i];
        }

        packetCount--;
    }

    // Wait until all 8 bytes are received
    if (packetCount < PACKET_SIZE)
    {
        return;
    }

    // Validate checksum
    uint8_t calculatedChecksum = calculateChecksum(packet);

    if (calculatedChecksum == packet[7])
    {
        // Valid packet: send ACK byte to Windows
        Serial.write(ACK_BYTE);
    }

    else
    {
        // Invalid packet: do not send ACK
        Serial.println("Invalid checksum");
    }

    // Reset packet buffer
    packetCount = 0;
}


void setup()
{
    Serial.begin(115200);

    packetCount = 0;
}


void loop()
{
    // Read all currently available UART bytes
    while (Serial.available() > 0)
    {
        uint8_t receivedByte = Serial.read();

        processIncomingByte(receivedByte);
    }
}
