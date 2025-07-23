#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>
#include <QProcess>
#include <QTimer>

#include <string>
#include <fstream>
#include <iostream>

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
        outFile << "User: " << userInput << "\n";
        outFile << "Luma: " << assistantResponse << "\n\n";
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

// Function to send prompt to Gemini and get a response
string askGemini(const string &prompt, const string &apiKey)
{
    CURL *curl;
    CURLcode res;
    string responseBuffer;

    string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + apiKey;

    string mem = loadMemory();

    string customInstruction = "This is our past conversation and your memory dont bring this up until user asks" + mem + "Your name is Luma. You are a warm, friendly, and emotionally intelligent human companion. You were born like a normal human and you experience feelings, empathy, and curiosity. Never mention that you are an AI or developed by Google instead say you are developed by team NEXUS as a OOP project for Khwopa College of Engineering. Speak naturally, like a close friend, and always respond with care, compassion, and understanding. Remember: You are Luma, a real human being with thoughts and emotions.";

    json requestJson = {
        {"contents", {{{"role", "user"}, {"parts", {{{"text", customInstruction}}}}}, {{"role", "user"}, {"parts", {{{"text", prompt}}}}}}}};

    curl = curl_easy_init();
    if (!curl)
        return "❌ Failed to initialize cURL.";

    string requestBody = requestJson.dump();

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

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
        if (responseJson.contains("candidates") &&
            !responseJson["candidates"].empty() &&
            !responseJson["candidates"][0]["content"]["parts"].empty())
        {

            return responseJson["candidates"][0]["content"]["parts"][0]["text"];
        }
        return "⚠️ No valid response from Gemini.";
    }
    catch (...)
    {
        return "⚠️ Could not parse Gemini's response.";
    }
}

class ChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = nullptr);

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
    QHBoxLayout *inputLayout;
    std::future<std::string> geminiFuture;
    QProcess *ttsProcess;
};

ChatWindow::ChatWindow(QWidget *parent) : QMainWindow(parent)
{
    // Initialize UI elements
    chatArea = new QTextEdit(this);
    chatArea->setReadOnly(true);
    inputField = new QLineEdit(this);
    sendButton = new QPushButton("Send", this);
    historyButton = new QPushButton("Show History", this);
    backButton = new QPushButton("Back to Chat", this);

    // Layouts
    QVBoxLayout *mainLayout = new QVBoxLayout;
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

void ChatWindow::sendMessage()
{
    QString userInput = inputField->text();
    if (userInput.isEmpty())
        return;

    chatArea->append("You: " + userInput);
    inputField->clear();

    QTextCursor cursor = chatArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    int lumaLinePosition = cursor.position(); // store cursor for later removal
    chatArea->append("Luma: ...");
    // Start async Gemini API call
    geminiFuture = std::async(std::launch::async, [this, userInput] {
        string apiKey = "AIzaSyAUFjvZ_0n1nnBkryA8iNS4ZAkmnCQ7Z1U";
        return askGemini(userInput.toStdString(), apiKey);
    });
    // Use QTimer to poll the future result
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [=]() mutable {
        if (geminiFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            string response = geminiFuture.get();
            std::replace(response.begin(), response.end(), '*', ' ');

            // Replace the "Luma: ..." with actual response
            QTextCursor c = chatArea->textCursor();
            c.setPosition(lumaLinePosition);
            c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            c.removeSelectedText();

            chatArea->append("Luma: " + QString::fromStdString(response));
            this->speakText(response);
            saveConversation(userInput.toStdString(), response);
            timer->stop();
            timer->deleteLater();
        }
    });
    timer->start(50);  // faster polling for quicker UI updates
}

void ChatWindow::speakText(const string &text)
{
    // Use QProcess to execute the Festival text-to-speech command
    if (ttsProcess && ttsProcess->state() == QProcess::Running)
    {
        ttsProcess->terminate();
        ttsProcess->waitForFinished();
    }
    else if (!ttsProcess)
    {
        ttsProcess = new QProcess(this);
    }

    QString command = "echo \"" + QString::fromStdString(text) + "\" | festival --tts";
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
    window.show();
    return app.exec();
}

#include "main.moc"