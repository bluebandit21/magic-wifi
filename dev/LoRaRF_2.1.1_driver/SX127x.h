#ifndef _SX127X_H_
#define _SX127X_H_

#include <BaseLoRa.h>
#include <SX127x_driver.h>

// TX and RX operation status
#define SX127X_STATUS_DEFAULT                   LORA_STATUS_DEFAULT
#define SX127X_STATUS_TX_WAIT                   LORA_STATUS_TX_WAIT
#define SX127X_STATUS_TX_TIMEOUT                LORA_STATUS_TX_TIMEOUT
#define SX127X_STATUS_TX_DONE                   LORA_STATUS_TX_DONE
#define SX127X_STATUS_RX_WAIT                   LORA_STATUS_RX_WAIT
#define SX127X_STATUS_RX_CONTINUOUS             LORA_STATUS_RX_CONTINUOUS
#define SX127X_STATUS_RX_TIMEOUT                LORA_STATUS_RX_TIMEOUT
#define SX127X_STATUS_RX_DONE                   LORA_STATUS_RX_DONE
#define SX127X_STATUS_HEADER_ERR                LORA_STATUS_HEADER_ERR
#define SX127X_STATUS_CRC_ERR                   LORA_STATUS_CRC_ERR
#define SX127X_STATUS_CAD_WAIT                  LORA_STATUS_CAD_WAIT
#define SX127X_STATUS_CAD_DETECTED              LORA_STATUS_CAD_DETECTED
#define SX127X_STATUS_CAD_DONE                  LORA_STATUS_CAD_DONE

#if defined(USE_LORA_SX126X) && defined(USE_LORA_SX127X)
class SX127x : public BaseLoRa
#else
class SX127x
#endif
{
    public:

        SX127x(LORA port);

        // Common Operational methods
        bool begin();
        void end();
        bool reset();
        void sleep();
        void wake();
        void standby();

        // Hardware configuration methods
        void setSPI(EUSCI_B_SPI_initMasterParam &SpiObject);
        void setCurrentProtection(uint8_t current);
        void setOscillator(uint8_t option);

        // Modem, modulation parameter, and packet parameter setup methods
        void setModem(uint8_t modem=SX127X_LORA_MODEM);
        void setFrequency(uint32_t frequency);
        void setTxPower(uint8_t txPower, uint8_t paPin=SX127X_TX_POWER_PA_BOOST);
        void setRxGain(uint8_t boost, uint8_t level=SX127X_RX_GAIN_AUTO);
        void setLoRaModulation(uint8_t sf, uint32_t bw, uint8_t cr, bool ldro=false);
        void setLoRaPacket(uint8_t headerType, uint16_t preambleLength, uint8_t payloadLength, bool crcType=false, bool invertIq=false);
        void setSpreadingFactor(uint8_t sf);
        void setBandwidth(uint32_t bw);
        void setCodeRate(uint8_t cr);
        void setLdroEnable(bool ldro=true);
        void setHeaderType(uint8_t headerType);
        void setPreambleLength(uint16_t preambleLength);
        void setPayloadLength(uint8_t payloadLength);
        void setCrcEnable(bool crcType=true);
        void setInvertIq(bool invertIq=true);
        void setSyncWord(uint16_t syncWord);

        // Transmit related methods
        void beginPacket();
        bool endPacket(uint32_t timeout=0);
        void write(uint8_t data);
        void write(uint8_t* data, uint8_t length);
        void write(char* data, uint8_t length);
        template <typename T> void put(T data)
        {
            const uint8_t length = sizeof(T);
            union conv {
                T Data;
                uint8_t Binary[length];
            };
            union conv u;
            u.Data = data;
            write(u.Binary, length);
        }
        void onTransmit(void(&callback)());

        // Receive related methods
        bool request(uint32_t timeout=SX127X_RX_SINGLE);
        uint8_t available();
        uint8_t read();
        uint8_t read(uint8_t* data, uint8_t length);
        uint8_t read(char* data, uint8_t length);
        void purge(uint8_t length=0);
        template <typename T> uint8_t get(T &data)
        {
            const uint8_t length = sizeof(T);
            union conv {
                T Data;
                uint8_t Binary[length];
            };
            union conv u;
            uint8_t len = read(u.Binary, length);
            data = u.Data;
            return len;
        }
        void onReceive(void(&callback)());

        // Wait, operation status, and packet status methods
        bool wait(uint32_t timeout=0);
        uint8_t status();
        uint32_t transmitTime();
        float dataRate();
        int16_t packetRssi();
        float snr();
        int16_t rssi();
        uint32_t random();

    protected:

        uint8_t _modem;
        uint32_t _frequency;
        uint8_t _sf = 7;
        uint32_t _bw = 125000;
        uint8_t _headerType;
        uint8_t _payloadLength;
         void (*_onTransmit)();
         void (*_onReceive)();

    private:

        EUSCI_B_SPI_initMasterParam _spi;
        LORA device;
        int8_t _irq = 0; // change to enable/disable
        uint8_t _statusWait;
        volatile uint8_t _statusIrq = 0xFF;
        uint32_t _transmitTime  = 0;
        uint8_t _payloadTxRx = 0;
        int8_t _irqStatic;
        //static int8_t _pinToLow;
        uint16_t _random;

        // Interrupt handler methods
        void _interruptTx(void);
        void _interruptRx();
        void _interruptRxContinuous();

public:
         void (SX127x::*curr_callback)( void );


private:

         uint16_t baseAddress = SX127X_SPI_0;
         EUSCI_B_SPI_initMasterParam sx127x_spi_params;
         uint32_t sx127x_spiFrequency = SX127X_SPI_FREQUENCY;
         int8_t sx127x_nss_port;
         int8_t sx127x_nss_pin;
         int8_t sx127x_reset_port;
         int8_t sx127x_reset_pin;


         void sx127x_setSPI(EUSCI_B_SPI_initMasterParam &SpiObject, LORA port);
         //void sx127x_setPins(int8_t nss);
         void sx127x_reset();
         void sx127x_begin(LORA port);

         // SX126x driver: Register access functions
         void sx127x_writeBits(uint8_t address, uint8_t data, uint8_t position, uint8_t length);
         void sx127x_writeRegister(uint8_t address, uint8_t data);
         uint8_t sx127x_readRegister(uint8_t address);
         uint8_t sx127x_transfer(uint8_t address, uint8_t data);
         void sx127x_interruptEnable(LORA port);
         void sx127x_interruptDisable(LORA port);

};

#endif
