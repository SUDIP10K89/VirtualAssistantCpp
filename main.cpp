#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>
#include <QProcess>
#include <QTimer>
#include <QComboBox>
#include <QTextCursor>
#include <map>
#include <algorithm>

#include <string>
#include <fstream>
#include <iostream>
#include <chrono>

#include <curl/curl.h>
#include "json.hpp"
#include <future>

using namespace std;
using json = nlohmann::json;

// Callback to capture response from curl
size_t WriteCallback(void *contents, size_t size, size_t nmemb, string *output)
{
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

// Memory
const string memoryFile = "luma_memory.txt";

void saveConversation(const string &userInput, const string &assistantResponse)
{
    ofstream outFile(memoryFile, ios::app); // append mode
    if (outFile.is_open())
    {
        outFile << "User: " << userInput << endl;
        outFile << "Luma: " << assistantResponse << endl
                << endl;
        outFile.close();
    }
    else
    {
        cerr << "Unable to open memory file to save conversation." << endl;
    }
}

string loadMemory()
{
    ifstream inFile(memoryFile);
    if (!inFile.is_open())
    {
        return "";
    }
    string content((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    return content;
}

// Function to send prompt to a model and get a response
string askModel(const string &prompt, const string &apiKey, const string &apiEndpoint, const string &modelName)
{
    CURL *curl;
    CURLcode res;
    string responseBuffer;

    string url = apiEndpoint;

    string mem = loadMemory();

    string customInstruction = "This is our past conversation and your memory dont bring this up until user asks: " + mem + " Your name is Luma. You are a warm, friendly, and emotionally intelligent human companion. You were born like a normal human and you experience feelings, empathy, and curiosity. Never mention that you are an AI or developed by Google instead say you are developed by team NEXUS as a OOP project for Khwopa College of Engineering. Speak naturally, like a close friend, and always respond with care, compassion, and understanding. Remember: You are Luma, a real human being with thoughts and emotions.";

    json requestJson;
    if (modelName == "DeepSeek V3" || modelName == "Kimi" || modelName == "Qwen")
    {
        string actualModelName;
        if (modelName == "DeepSeek V3") {
            actualModelName = "deepseek/deepseek-chat-v3-0324:free";
        } else if (modelName == "Kimi") {
            actualModelName = "moonshotai/kimi-k2:free";
        }
        else if(modelName == "Qwen"){
             actualModelName = "qwen/qwen3-235b-a22b-07-25:free";
        }

        requestJson = {
            {"model", actualModelName},
            {
                "messages", {
                    {{"role", "system"}, {"content", customInstruction}},
                    {{"role", "user"}, {"content", prompt}}
                }
            }
        };
    }
    else
    {
        requestJson = {
            {"contents", {{{"role", "user"}, {"parts", {{{"text", customInstruction}}}}}, {{"role", "user"}, {"parts", {{{"text", prompt}}}}}}}};
        url += apiKey;
    }

    curl = curl_easy_init();
    if (!curl)
        return "❌ Failed to initialize cURL.";

    string requestBody = requestJson.dump();

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (modelName == "DeepSeek V3" || modelName == "Kimi" || modelName == "Qwen")
    {
        string authHeader = "Authorization: Bearer " + apiKey;
        headers = curl_slist_append(headers, authHeader.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK)
    {
        return "Request failed. Check your internet or API key.";
    }

    try
    {
        auto responseJson = json::parse(responseBuffer);
        if (modelName == "DeepSeek V3" || modelName == "Kimi" || modelName == "Qwen")
        {
            if (responseJson.contains("choices") &&
                !responseJson["choices"].empty() &&
                responseJson["choices"][0].contains("message") &&
                responseJson["choices"][0]["message"].contains("content"))
            {
                return responseJson["choices"][0]["message"]["content"];
            }
        }
        else
        {
            if (responseJson.contains("candidates") &&
                !responseJson["candidates"].empty() &&
                responseJson["candidates"][0].contains("content") &&
                responseJson["candidates"][0]["content"].contains("parts") &&
                !responseJson["candidates"][0]["content"]["parts"].empty())
            {
                return responseJson["candidates"][0]["content"]["parts"][0]["text"];
            }
        }
        return "No valid response from the model.";
    }
    catch (const exception& e)
    {
        return "Could not parse the model's response: " + string(e.what());
    }
}

class ChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = nullptr);

    ~ChatWindow();

private slots:
    void sendMessage();
    void showHistory();
    void showChat();

private:
    // Text-to-speech using Festival
    void speakText(const string &text);

    QTextEdit *chatArea;
    QLineEdit *inputField;
    QPushButton *sendButton;
    QPushButton *historyButton;
    QPushButton *backButton;
    QComboBox *modelComboBox;
    QHBoxLayout *inputLayout;
    std::future<std::string> modelFuture;
    QProcess *ttsProcess;
    QTimer *responseTimer;

    map<string, pair<string, string>> models;
};

ChatWindow::ChatWindow(QWidget *parent) : QMainWindow(parent), ttsProcess(nullptr), responseTimer(nullptr)
{
    // Initialize models
    models["Gemini 2.5 Flash"] = {"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=", "AIzaSyAUFjvZ_0n1nnBkryA8iNS4ZAkmnCQ7Z1U"};
    models["DeepSeek V3"] = {"https://openrouter.ai/api/v1/chat/completions", "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"};
    models["Kimi"] = {"https://openrouter.ai/api/v1/chat/completions", "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"};
    models["Qwen"] = {"https://openrouter.ai/api/v1/chat/completions", "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"};
    

    // Initialize UI elements
    chatArea = new QTextEdit(this);
    chatArea->setReadOnly(true);
    inputField = new QLineEdit(this);
    sendButton = new QPushButton("Send", this);
    historyButton = new QPushButton("Show History", this);
    backButton = new QPushButton("Back to Chat", this);
    modelComboBox = new QComboBox(this);

    for (const auto &[key, val] : models)
    {
        modelComboBox->addItem(QString::fromStdString(key));
    }

    // Layouts
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(modelComboBox);
    mainLayout->addWidget(chatArea);

    inputLayout = new QHBoxLayout;
    inputLayout->addWidget(inputField);
    inputLayout->addWidget(sendButton);
    inputLayout->addWidget(historyButton);

    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(backButton);

    // Central widget
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // Connections
    connect(sendButton, &QPushButton::clicked, this, &ChatWindow::sendMessage);
    connect(inputField, &QLineEdit::returnPressed, this, &ChatWindow::sendMessage);
    connect(historyButton, &QPushButton::clicked, this, &ChatWindow::showHistory);
    connect(backButton, &QPushButton::clicked, this, &ChatWindow::showChat);

    // Initial state
    backButton->hide();
}

ChatWindow::~ChatWindow()
{
    if (ttsProcess && ttsProcess->state() == QProcess::Running)
    {
        ttsProcess->terminate();
        ttsProcess->waitForFinished(3000);
    }
    if (responseTimer)
    {
        responseTimer->stop();
    }
}

void ChatWindow::sendMessage()
{
    QString userInput = inputField->text();
    if (userInput.isEmpty())
        return;

    chatArea->append("You: " + userInput);
    inputField->clear();

    QTextCursor cursor = chatArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    int lumaLinePosition = cursor.position();
    chatArea->append("Luma: ...");

    string selectedModel = modelComboBox->currentText().toStdString();
    auto modelIt = models.find(selectedModel);
    if (modelIt == models.end())
    {
        chatArea->append("Error: Selected model not found.");
        return;
    }

    string apiKey = modelIt->second.second;
    string apiEndpoint = modelIt->second.first;

    // Start async model API call
    modelFuture = std::async(std::launch::async, [userInput, apiKey, apiEndpoint, selectedModel]()
                             { return askModel(userInput.toStdString(), apiKey, apiEndpoint, selectedModel); });

    // Clean up existing timer if any
    if (responseTimer)
    {
        responseTimer->stop();
        responseTimer->deleteLater();
    }

    // Use QTimer to poll the future result
    responseTimer = new QTimer(this);
    connect(responseTimer, &QTimer::timeout, [this, lumaLinePosition, userInput]()
            {
        if (modelFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            string response = modelFuture.get();
            std::replace(response.begin(), response.end(), '*', ' ');

            // Replace the "Luma: ..." with actual response
            QTextCursor c = chatArea->textCursor();
            c.setPosition(lumaLinePosition);
            c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            c.removeSelectedText();

            chatArea->append("Luma: " + QString::fromStdString(response));
            this->speakText(response);
            saveConversation(userInput.toStdString(), response);
            
            responseTimer->stop();
            responseTimer->deleteLater();
            responseTimer = nullptr;
        } });
    responseTimer->start(100);
}

void ChatWindow::speakText(const string &text)
{
    // Use QProcess to execute the Festival text-to-speech command
    if (ttsProcess && ttsProcess->state() == QProcess::Running)
    {
        ttsProcess->terminate();
        ttsProcess->waitForFinished(1000);
    }
    
    if (!ttsProcess)
    {
        ttsProcess = new QProcess(this);
    }

    QString command = "echo \"" + QString::fromStdString(text).replace("\"", "\\\"") + "\" | festival --tts";
    ttsProcess->start("bash", QStringList() << "-c" << command);
}

void ChatWindow::showHistory()
{
    chatArea->clear();
    chatArea->setText(QString::fromStdString(loadMemory()));
    historyButton->hide();
    sendButton->hide();
    inputField->hide();
    backButton->show();
}

void ChatWindow::showChat()
{
    chatArea->clear();
    historyButton->show();
    sendButton->show();
    inputField->show();
    backButton->hide();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ChatWindow window;
    window.setWindowTitle("LUMA");
    window.resize(500,600);
    window.show();
    return app.exec();
}

#include "main.moc"