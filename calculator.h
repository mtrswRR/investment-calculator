// calculator.h
#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QDate>
#include <QCompleter>

class Calculator : public QWidget {
    Q_OBJECT

public:
    Calculator(QWidget *parent = nullptr);

private slots:
    void calculate();
    void onQuoteFinished(QNetworkReply *reply);
    void onChartFinished(QNetworkReply *reply);

private:
    void fetchQuote();

    QLineEdit *symbolEdit;
    QLineEdit *amountEdit;
    QLineEdit *yearsEdit;
    QLineEdit *returnEdit;

    QLabel *priceLabel;
    QLabel *sharesLabel;
    QLabel *calcReturnLabel;
    QLabel *futurePriceLabel;
    QLabel *futureValueLabel;

    QNetworkAccessManager *manager;

    double currentPrice;
    double annualReturn;
    bool useHistorical;
    QString symbol;
    double amount;
    double years;
};

#endif // CALCULATOR_H