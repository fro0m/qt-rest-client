#include "mainwindowe.h"
#include <QtWidgets/QDesktopWidget>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

MainWindowe::MainWindowe(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("rest client");
    QDesktopWidget dw;
    setFixedSize(dw.width() * 0.5, dw.height() * 0.7);


    //QObject::connect(reply, &QNetworkReply::readyRead,     this, &MainWindowe::updateText );


    buttonInfo = new QPushButton("Info");
    connect(buttonInfo, &QPushButton::released, this, &MainWindowe::getInfo);

    buttonAutentificate = new QPushButton("Autentificate");
    connect(buttonAutentificate, &QPushButton::released, this, &MainWindowe::postAutenticate);

    butonUserInfo = new QPushButton("User Info");
    connect(butonUserInfo, &QPushButton::released, this, &MainWindowe::getUserInfo);

    buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(buttonInfo);
    buttonLayout->addWidget(buttonAutentificate);
    buttonLayout->addWidget(butonUserInfo);

    text = new QTextEdit();
    text->setPlainText("No data yet");

    mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(text);

    QWidget* centralWidget = new QWidget();
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    token = QByteArray();

}

MainWindowe::~MainWindowe()
{

}

void MainWindowe::getInfo()
{
    QString urlStr = "http://stas.pubsandbox.eterhost.ru/rest-test/public/api/info";

    HttpRequestInput input(urlStr, "GET");

    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(executionFinished(HttpRequestWorker*)), this, SLOT(handleResult(HttpRequestWorker*)));

    worker->execute(&input);
}

void MainWindowe::postAutenticate()
{
    QString urlStr = "http://stas.pubsandbox.eterhost.ru/rest-test/public/api/authenticate";

    HttpRequestInput input(urlStr, "POST");

    input.addVar("email", "testuser");
    input.addVar("password", "testuser");

    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(executionFinished(HttpRequestWorker*)), this, SLOT(handle_autentification(HttpRequestWorker*)));
    worker->execute(&input);
}

void MainWindowe::getUserInfo()
{
    QString urlStr = "http://stas.pubsandbox.eterhost.ru/rest-test/public/api/userinfo";

    HttpRequestInput input(urlStr, "GET");
    //QString authorizationValueString = QString("Bearer %1").arg(token);
    QByteArray authorizationValue = QByteArray("Bearer ");
    authorizationValue.append(token);

    input.addHeader("Authorization", authorizationValue);

    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(executionFinished(HttpRequestWorker*)), this, SLOT(handleResult(HttpRequestWorker*)));

    worker->execute(&input);
}

void MainWindowe::handleResult(HttpRequestWorker * worker_)
{
    worker_->deleteLater();
    if (worker_->errorType != QNetworkReply::NoError) {
        qDebug() << worker_->errorStr;
        updateText(worker_->errorStr);
        //delete worker_;
        return;
    }
    QString result = worker_->response;
    worker_->deleteLater();
    //delete worker_;

    qDebug() << result;
    updateText(result);
}

void MainWindowe::handle_autentification(HttpRequestWorker *worker_)
{
    worker_->deleteLater();
    QByteArray result = worker_->response;

    QJsonDocument responseJson = QJsonDocument::fromJson(result/*, QJsonParseError *error = Q_NULLPTR*/); //TODO add error check
    QJsonObject responseJsonObject = responseJson.object();
    QJsonValue tokenJsonValue = responseJsonObject.value(QString("token"));
    if (tokenJsonValue.type() == QJsonValue::Undefined) {
        updateText("there is no token in response");
        //delete worker_;
        return;
    }

    token = tokenJsonValue.toString().toLatin1();
    if (token.isNull()) {
        updateText("token contains illegal non-Latin1 characters");
        //delete worker_;
        return;
    }

    //delete worker_;

    qDebug() << token;
    updateText(result);
}


void MainWindowe::updateText(const QString& text_)
{
    text->append(QString("\r%1").arg(text_));
}
