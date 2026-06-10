#include <iostream>
#include "client/ollama_client.h"

int main() {
    std::cout << "OopAgent started!" << std::endl;

    OllamaClient client("http://localhost:11434", "gemma3");
    client.setTemperature(0.7f);

    std::vector<Message> messages = {
        {"user", "Say hello in one sentence"}
    };

    try {
        std::string response = client.chat(messages);
        std::cout << "Response: " << response << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    return 0;
}