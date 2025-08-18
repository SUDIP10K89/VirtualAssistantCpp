

# LUMA – AI Chat Application

LUMA is a **Qt-based AI chat assistant** written in C++.
It integrates multiple AI models (Gemini, DeepSeek, Kimi, Qwen, GPT) via APIs, provides a **chat UI**, supports **conversation history persistence**, and features **text-to-speech (TTS)** using `festival`.

---

## ✨ Features

* 🖥️ **Modern GUI** built with **Qt**
* 🤖 **Multiple AI model support** (Gemini, DeepSeek, Kimi, Qwen, GPT)
* 💾 **Persistent memory** – conversations saved locally (`luma_memory.txt`)
* 🔊 **Text-to-Speech (TTS)** integration with `festival`
* 📜 **Conversation history view**
* ⚡ **Async API requests** with `std::future`
* 🌐 Uses **cURL** + `nlohmann/json` for API communication

---

## 🚀 Getting Started

### 1. Prerequisites

Make sure you have the following installed:

* **C++17 (or higher) compiler**
* **Qt5 / Qt6** (for GUI)
* **cURL** (libcurl)
* **nlohmann/json** (header-only JSON library)
* **Festival TTS** (optional, for voice output)

On Ubuntu/Debian:

```bash
sudo apt-get install qtbase5-dev libcurl4-openssl-dev festival
```

### 2. Clone Repository

```bash
git clone https://github.com/SUDIP10K89/VirtualAssistantCpp
cd luma-chat
```

### 3. Build Project

Using **qmake + make**:

```bash
qmake
make
```


### 4. Run

```bash
./luma-chat
```

---

## ⚙️ Configuration

### API Keys

LUMA comes pre-configured with multiple model endpoints.
Inside `ChatWindow::ChatWindow()`, you’ll find model API mappings:

```cpp
models["Gemini 2.5 Flash"] = {"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=", "<YOUR_API_KEY>"};
models["DeepSeek V3"]      = {"https://openrouter.ai/api/v1/chat/completions", "<YOUR_API_KEY>"};
models["Kimi"]             = {"https://openrouter.ai/api/v1/chat/completions", "<YOUR_API_KEY>"};
models["Qwen"]             = {"https://openrouter.ai/api/v1/chat/completions", "<YOUR_API_KEY>"};
models["GPT"]              = {"https://openrouter.ai/api/v1/chat/completions", "<YOUR_API_KEY>"};
```

👉 Replace `<YOUR_API_KEY>` with your actual API keys.

---

## 📂 Project Structure

```
├── main.cpp          # Core application source
├── main.moc          # Qt MOC file
├── luma_memory.txt   # Conversation history (generated at runtime)
├── README.md         # Project documentation
```

---

## 🛠️ Tech Stack

* **C++17**
* **Qt5/6 (QApplication, QMainWindow, QWidgets, QTimer, QProcess, QComboBox, etc.)**
* **cURL** for HTTP requests
* **nlohmann/json** for JSON handling
* **Festival** for TTS

---



# VirtualAssistantCpp

Festival for text to speech
Gemini Api
Curl for http request

//g++ ai_assistant.cpp -o assistant -lcurl

//qmake && make
//qmake main.pro
//make
