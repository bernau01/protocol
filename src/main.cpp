
#include <protocol/data/dgbuff.hpp>
#include <cstdio>
#include <chrono>

#include "device.hpp"
#include "logger.hpp"

void printBuffer(const protocol::DgBuff& buff);

int main(int argc, char** argv) 
{
    itsr::logger_init();
    device::init();

    while(true) {
        device::update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // protocol::StaticDgBuff<2048> buffer;
    // protocol::DgBuff buff = buffer.sub(0,32).getValue();
    // protocol::eth::PacketField packet(buff);
    // protocol::eth::TxComposer tx_composer(buffer);

    // if(auto ret = tx_composer.init(1,1); ret != protocol::Status::OK) {
    //     printf("Failed to init the packet: %d\n", static_cast<int>(ret));
    //     return 1;
    // }

    // protocol::StaticDgBuff<9> node_buff;
    // node_buff[0] = 0xAA;
    // node_buff[1] = 0xB6;
    // node_buff[2] = 0xE5;
    // node_buff[3] = 0x01;
    // node_buff[4] = 0x00;

    // device::init();
    // device::update();

    // if(auto ret = tx_composer.place(node_buff); ret != protocol::Status::OK) {
    //     printf("Failed to place node buffer: %d\n", static_cast<int>(ret));
    //     return 1;
    // }

    // if(auto ret = tx_composer.startTransmission(); ret != protocol::Status::OK) {
    //     printf("Failed to start transmission: %d\n", static_cast<int>(ret));
    //     return 1;
    // }

    // printBuffer(tx_composer.getFinalBuff());

    // tx_composer.endTransmissionAndReset();

    // return packet.isPreambleValid() ? 0 : 1;
}

void printBuffer(const protocol::DgBuff& buff) {
    std::putchar('\n');
    for (auto i = 0; i < 16; ++i) {
        std::printf("%-2d ", i);
        if((i + 1) % 4 == 0) {
            std::printf("  ");
        }
    }
    std::printf("\n");
    std::printf("—————————————————————————————————————————————————————\n");

    for (size_t i = 0; i < buff.size(); ++i) {
        std::printf("%02X ", buff[i]);
        if((i + 1) % 16 == 0) {
            std::printf("\n");
        }
        else if((i + 1) % 4 == 0) {
            std::printf("  ");
        }
    }
    std::printf("\n");
}

