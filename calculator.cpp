// calculator.cpp
#include "calculator.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <QUrl>
#include <QNetworkRequest>
#include <QStringList>

Calculator::Calculator(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Калькулятор инвестиций");

    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *symbolLabel = new QLabel("Символ акции (например, SBER для Сбера):");
    layout->addWidget(symbolLabel);
    symbolEdit = new QLineEdit("SBER");
    layout->addWidget(symbolEdit);

    // Полный список топ-акций для автодополнения
    QStringList stocks = {"SBER", "GAZP", "LKOH", "VTBR", "YNDX", "ROSN", "AFLT", "GMKN", "TATN", "MGNT", "MOEX", "NVTK", "PLZL", "RSTI", "SNGSP", "TRNFP"};
    stocks.sort();  // Сортировка по алфавиту для "ближайших" (первых по порядку)
    QCompleter *completer = new QCompleter(stocks, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);  // Показывать акции, содержащие букву где угодно
    completer->setMaxVisibleItems(5);  // Всегда показывать ровно 5 подсказок (или меньше, если совпадений мало)
    symbolEdit->setCompleter(completer);

    QLabel *amountLabel = new QLabel("Сумма инвестиций (RUB):");
    layout->addWidget(amountLabel);
    amountEdit = new QLineEdit;
    layout->addWidget(amountEdit);

    QLabel *yearsLabel = new QLabel("Годы:");
    layout->addWidget(yearsLabel);
    yearsEdit = new QLineEdit;
    layout->addWidget(yearsEdit);

    QLabel *returnLabel = new QLabel("Ожидаемая годовая доходность (%) (опционально):");
    layout->addWidget(returnLabel);
    returnEdit = new QLineEdit;
    layout->addWidget(returnEdit);

    QPushButton *calcButton = new QPushButton("Рассчитать");
    layout->addWidget(calcButton);
    connect(calcButton, &QPushButton::clicked, this, &Calculator::calculate);

    priceLabel = new QLabel("Текущая цена: ");
    layout->addWidget(priceLabel);

    sharesLabel = new QLabel("Количество акций: ");
    layout->addWidget(sharesLabel);

    calcReturnLabel = new QLabel("Расчетная доходность: ");
    layout->addWidget(calcReturnLabel);

    futurePriceLabel = new QLabel("Предполагаемая будущая цена: ");
    layout->addWidget(futurePriceLabel);

    futureValueLabel = new QLabel("Будущая стоимость: ");
    layout->addWidget(futureValueLabel);

    setLayout(layout);

    manager = new QNetworkAccessManager(this);
}

void Calculator::calculate() {
    symbol = symbolEdit->text().toUpper();
    QString amountStr = amountEdit->text();
    QString yearsStr = yearsEdit->text();
    QString returnStr = returnEdit->text();

    bool ok;
    amount = amountStr.toDouble(&ok);
    if (!ok || amount <= 0) {
        QMessageBox::warning(this, "Ошибка", "Неверная сумма инвестиций");
        return;
    }

    years = yearsStr.toDouble(&ok);
    if (!ok || years <= 0) {
        QMessageBox::warning(this, "Ошибка", "Неверное количество лет");
        return;
    }

    useHistorical = returnStr.isEmpty();
    if (!useHistorical) {
        annualReturn = returnStr.toDouble(&ok) / 100.0;
        if (!ok) {
            QMessageBox::warning(this, "Ошибка", "Неверная доходность");
            return;
        }
        calcReturnLabel->setText(QString("Расчетная доходность: %1%").arg(annualReturn * 100, 0, 'f', 2));
    } else {
        calcReturnLabel->setText("Расчетная доходность: (вычисляется из исторических данных)");
    }

    priceLabel->setText("Текущая цена: ");
    sharesLabel->setText("Количество акций: ");
    futurePriceLabel->setText("Предполагаемая будущая цена: ");
    futureValueLabel->setText("Будущая стоимость: ");

    if (useHistorical) {
        QString chartUrl = QString("https://iss.moex.com/iss/history/engines/stock/markets/shares/boards/TQBR/securities/%1.json?limit=365").arg(symbol);
        QNetworkRequest request = QNetworkRequest(QUrl(chartUrl));
        request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
        QNetworkReply *reply = manager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() { onChartFinished(reply); });
    } else {
        fetchQuote();
    }
}

void Calculator::onChartFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "Предупреждение", "Ошибка при получении исторических данных: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        QMessageBox::warning(this, "Предупреждение", "Неверный формат данных");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    QJsonObject history = obj["history"].toObject();
    QJsonArray dataArray = history["data"].toArray();
    if (dataArray.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Нет исторических данных для символа");
        reply->deleteLater();
        return;
    }

    QJsonArray columns = history["columns"].toArray();
    int closeIndex = -1;
    int dateIndex = -1;
    for (int i = 0; i < columns.size(); ++i) {
        QString col = columns[i].toString();
        if (col == "CLOSE") closeIndex = i;
        if (col == "TRADEDATE") dateIndex = i;
    }

    if (closeIndex == -1 || dateIndex == -1) {
        QMessageBox::warning(this, "Предупреждение", "Недостаточно данных для расчета");
        reply->deleteLater();
        return;
    }

    double firstClose = dataArray[0].toArray()[closeIndex].toDouble();
    double lastClose = dataArray[dataArray.size() - 1].toArray()[closeIndex].toDouble();

    QDate firstDate = QDate::fromString(dataArray[0].toArray()[dateIndex].toString(), "yyyy-MM-dd");
    QDate lastDate = QDate::fromString(dataArray[dataArray.size() - 1].toArray()[dateIndex].toString(), "yyyy-MM-dd");

    double days = firstDate.daysTo(lastDate);
    if (days <= 0) {
        QMessageBox::warning(this, "Предупреждение", "Неверные временные метки");
        reply->deleteLater();
        return;
    }

    double totalReturn = lastClose / firstClose;
    annualReturn = std::pow(totalReturn, 365.25 / days) - 1.0;

    calcReturnLabel->setText(QString("Расчетная доходность: %1%").arg(annualReturn * 100, 0, 'f', 2));

    reply->deleteLater();

    fetchQuote();
}

void Calculator::fetchQuote() {
    QString quoteUrl = QString("https://iss.moex.com/iss/engines/stock/markets/shares/boards/TQBR/securities/%1.json").arg(symbol);
    QNetworkRequest request = QNetworkRequest(QUrl(quoteUrl));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onQuoteFinished(reply); });
}

void Calculator::onQuoteFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "Предупреждение", "Ошибка при получении текущей цены: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        QMessageBox::warning(this, "Предупреждение", "Неверный формат данных");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    QJsonObject marketdata = obj["marketdata"].toObject();
    QJsonArray dataArray = marketdata["data"].toArray();
    if (dataArray.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "У данной акции отсутствует цена");
        reply->deleteLater();
        return;
    }

    QJsonArray columns = marketdata["columns"].toArray();
    int lastIndex = -1;
    for (int i = 0; i < columns.size(); ++i) {
        if (columns[i].toString() == "LAST") lastIndex = i;
    }

    if (lastIndex == -1) {
        QMessageBox::warning(this, "Предупреждение", "У данной акции отсутствует цена");
        reply->deleteLater();
        return;
    }

    currentPrice = dataArray[0].toArray()[lastIndex].toDouble(0.0);
    if (currentPrice == 0.0) {
        QMessageBox::warning(this, "Предупреждение", "У данной акции отсутствует цена");
        reply->deleteLater();
        return;
    }

    priceLabel->setText(QString("Текущая цена: %1 ₽").arg(currentPrice, 0, 'f', 2));

    double shares = amount / currentPrice;
    sharesLabel->setText(QString("Количество акций: %1").arg(shares, 0, 'f', 2));

    double power = std::pow(1 + annualReturn, years);
    double futurePrice = currentPrice * power;
    double futureValue = amount * power;

    futurePriceLabel->setText(QString("Предполагаемая будущая цена: %1 ₽").arg(futurePrice, 0, 'f', 2));
    futureValueLabel->setText(QString("Будущая стоимость: %1 ₽").arg(futureValue, 0, 'f', 2));

    reply->deleteLater();
}