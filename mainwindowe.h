#ifndef MAINWINDOWE_H
#define MAINWINDOWE_H

#include <QtWidgets/QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

#include "HttpRequestWorker.h"


class MainWindowe : public QMainWindow
{
    Q_OBJECT

public:
    MainWindowe(QWidget *parent = 0);
    ~MainWindowe();

private:
    QByteArray token;

private: // widgets
    QVBoxLayout* mainLayout;
    QHBoxLayout* buttonLayout;
    QTextEdit* text;
    QPushButton* buttonInfo;
    QPushButton* buttonAutentificate;
    QPushButton* butonUserInfo;
    //QSpacerItem* spacer;



public slots:
    void updateText(const QString& text_);
    void getInfo();
    void postAutenticate();
    void getUserInfo();


    void handleResult(HttpRequestWorker* worker_);
    void handle_autentification(HttpRequestWorker* worker_);
    //void handle_userInfo(HttpRequestWorker* worker_);
};

#endif // MAINWINDOWE_H
