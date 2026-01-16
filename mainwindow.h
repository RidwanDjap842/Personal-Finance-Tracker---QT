// ============================================
// FINANCE_TRACKER.pro (qmake project file)
// ============================================
// QT += core gui sql charts
//
// greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
//
// CONFIG += c++17
//
// SOURCES += \
//     main.cpp \
//     mainwindow.cpp
//
// HEADERS += \
//     mainwindow.h
//
// qnx: target.path = /tmp/${TARGET}/bin
// else: unix:!android: target.path = /opt/${TARGET}/bin
// !isEmpty(target.path): INSTALLS += target
// ============================================

// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>

QT_CHARTS_USE_NAMESPACE

    QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addTransaction();
    void deleteTransaction();
    void updateSummary();
    void filterByCategory();
    void filterByDateRange();
    void exportToCSV();
    void updateChart();

private:
    void setupDatabase();
    void setupUI();
    void loadTransactions();
    void calculateBalance();

    QSqlDatabase db;

    // UI Components
    QTableWidget *transactionTable;
    QLineEdit *amountEdit;
    QLineEdit *descriptionEdit;
    QComboBox *categoryCombo;
    QComboBox *typeCombo; // Income/Expense
    QDateEdit *dateEdit;
    QPushButton *addBtn;
    QPushButton *deleteBtn;
    QPushButton *exportBtn;

    QLabel *totalIncomeLabel;
    QLabel *totalExpenseLabel;
    QLabel *balanceLabel;

    QChartView *chartView;
    QChart *pieChart;

    double totalIncome;
    double totalExpense;
};

#endif // MAINWINDOW_H

// mainwindow.cpp
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), totalIncome(0), totalExpense(0)
{
    setupDatabase();
    setupUI();
    loadTransactions();
    updateSummary();
    updateChart();
}

MainWindow::~MainWindow()
{
    db.close();
}

void MainWindow::setupDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("finance.db");

    if (!db.open()) {
        QMessageBox::critical(this, "Database Error",
                              "Failed to open database: " + db.lastError().text());
        return;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS transactions ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "date TEXT NOT NULL, "
               "type TEXT NOT NULL, "
               "category TEXT NOT NULL, "
               "amount REAL NOT NULL, "
               "description TEXT)");

    // Insert sample categories if table is empty
    query.exec("CREATE TABLE IF NOT EXISTS categories ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT NOT NULL, "
               "type TEXT NOT NULL)");
}

void MainWindow::setupUI()
{
    setWindowTitle("Personal Finance Tracker");
    resize(1000, 700);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Summary Section
    QGroupBox *summaryGroup = new QGroupBox("Financial Summary");
    QHBoxLayout *summaryLayout = new QHBoxLayout();

    totalIncomeLabel = new QLabel("Income: $0.00");
    totalExpenseLabel = new QLabel("Expenses: $0.00");
    balanceLabel = new QLabel("Balance: $0.00");

    totalIncomeLabel->setStyleSheet("QLabel { color: green; font-size: 14pt; font-weight: bold; }");
    totalExpenseLabel->setStyleSheet("QLabel { color: red; font-size: 14pt; font-weight: bold; }");
    balanceLabel->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; }");

    summaryLayout->addWidget(totalIncomeLabel);
    summaryLayout->addWidget(totalExpenseLabel);
    summaryLayout->addWidget(balanceLabel);
    summaryLayout->addStretch();
    summaryGroup->setLayout(summaryLayout);
    mainLayout->addWidget(summaryGroup);

    // Input Section
    QGroupBox *inputGroup = new QGroupBox("Add Transaction");
    QHBoxLayout *inputLayout = new QHBoxLayout();

    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);

    typeCombo = new QComboBox();
    typeCombo->addItems({"Expense", "Income"});

    categoryCombo = new QComboBox();
    categoryCombo->addItems({"Food", "Transport", "Entertainment", "Bills",
                             "Shopping", "Salary", "Investment", "Other"});

    amountEdit = new QLineEdit();
    amountEdit->setPlaceholderText("Amount");

    descriptionEdit = new QLineEdit();
    descriptionEdit->setPlaceholderText("Description (optional)");

    addBtn = new QPushButton("Add");
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addTransaction);

    inputLayout->addWidget(new QLabel("Date:"));
    inputLayout->addWidget(dateEdit);
    inputLayout->addWidget(new QLabel("Type:"));
    inputLayout->addWidget(typeCombo);
    inputLayout->addWidget(new QLabel("Category:"));
    inputLayout->addWidget(categoryCombo);
    inputLayout->addWidget(new QLabel("Amount:"));
    inputLayout->addWidget(amountEdit);
    inputLayout->addWidget(new QLabel("Description:"));
    inputLayout->addWidget(descriptionEdit);
    inputLayout->addWidget(addBtn);

    inputGroup->setLayout(inputLayout);
    mainLayout->addWidget(inputGroup);

    // Chart Section
    pieChart = new QChart();
    pieChart->setTitle("Expenses by Category");
    pieChart->setAnimationOptions(QChart::SeriesAnimations);

    chartView = new QChartView(pieChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMaximumHeight(250);
    mainLayout->addWidget(chartView);

    // Transaction Table
    transactionTable = new QTableWidget();
    transactionTable->setColumnCount(6);
    transactionTable->setHorizontalHeaderLabels({"ID", "Date", "Type", "Category", "Amount", "Description"});
    transactionTable->horizontalHeader()->setStretchLastSection(true);
    transactionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    transactionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    transactionTable->hideColumn(0); // Hide ID column

    mainLayout->addWidget(transactionTable);

    // Action Buttons
    QHBoxLayout *actionLayout = new QHBoxLayout();
    deleteBtn = new QPushButton("Delete Selected");
    exportBtn = new QPushButton("Export to CSV");

    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteTransaction);
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportToCSV);

    actionLayout->addWidget(deleteBtn);
    actionLayout->addWidget(exportBtn);
    actionLayout->addStretch();

    mainLayout->addLayout(actionLayout);

    setCentralWidget(centralWidget);
}

void MainWindow::addTransaction()
{
    QString date = dateEdit->date().toString("yyyy-MM-dd");
    QString type = typeCombo->currentText();
    QString category = categoryCombo->currentText();
    double amount = amountEdit->text().toDouble();
    QString description = descriptionEdit->text();

    if (amount <= 0) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid amount.");
        return;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO transactions (date, type, category, amount, description) "
                  "VALUES (:date, :type, :category, :amount, :description)");
    query.bindValue(":date", date);
    query.bindValue(":type", type);
    query.bindValue(":category", category);
    query.bindValue(":amount", amount);
    query.bindValue(":description", description);

    if (query.exec()) {
        amountEdit->clear();
        descriptionEdit->clear();
        loadTransactions();
        updateSummary();
        updateChart();
    } else {
        QMessageBox::critical(this, "Database Error", query.lastError().text());
    }
}

void MainWindow::deleteTransaction()
{
    int row = transactionTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a transaction to delete.");
        return;
    }

    int id = transactionTable->item(row, 0)->text().toInt();

    QSqlQuery query;
    query.prepare("DELETE FROM transactions WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec()) {
        loadTransactions();
        updateSummary();
        updateChart();
    }
}

void MainWindow::loadTransactions()
{
    transactionTable->setRowCount(0);

    QSqlQuery query("SELECT * FROM transactions ORDER BY date DESC");

    while (query.next()) {
        int row = transactionTable->rowCount();
        transactionTable->insertRow(row);

        transactionTable->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
        transactionTable->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        transactionTable->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
        transactionTable->setItem(row, 3, new QTableWidgetItem(query.value(3).toString()));
        transactionTable->setItem(row, 4, new QTableWidgetItem(QString::number(query.value(4).toDouble(), 'f', 2)));
        transactionTable->setItem(row, 5, new QTableWidgetItem(query.value(5).toString()));
    }
}

void MainWindow::updateSummary()
{
    totalIncome = 0;
    totalExpense = 0;

    QSqlQuery query("SELECT type, SUM(amount) FROM transactions GROUP BY type");

    while (query.next()) {
        QString type = query.value(0).toString();
        double sum = query.value(1).toDouble();

        if (type == "Income") {
            totalIncome = sum;
        } else {
            totalExpense = sum;
        }
    }

    double balance = totalIncome - totalExpense;

    totalIncomeLabel->setText(QString("Income: $%1").arg(totalIncome, 0, 'f', 2));
    totalExpenseLabel->setText(QString("Expenses: $%1").arg(totalExpense, 0, 'f', 2));
    balanceLabel->setText(QString("Balance: $%1").arg(balance, 0, 'f', 2));

    if (balance >= 0) {
        balanceLabel->setStyleSheet("QLabel { color: green; font-size: 14pt; font-weight: bold; }");
    } else {
        balanceLabel->setStyleSheet("QLabel { color: red; font-size: 14pt; font-weight: bold; }");
    }
}

void MainWindow::updateChart()
{
    pieChart->removeAllSeries();

    QPieSeries *series = new QPieSeries();

    QSqlQuery query("SELECT category, SUM(amount) FROM transactions WHERE type='Expense' GROUP BY category");

    while (query.next()) {
        QString category = query.value(0).toString();
        double amount = query.value(1).toDouble();
        series->append(category + " ($" + QString::number(amount, 'f', 2) + ")", amount);
    }

    pieChart->addSeries(series);
    series->setLabelsVisible(true);
}

void MainWindow::exportToCSV()
{
    QString filename = QFileDialog::getSaveFileName(this, "Export to CSV", "", "CSV Files (*.csv)");

    if (filename.isEmpty()) return;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "Date,Type,Category,Amount,Description\n";

    QSqlQuery query("SELECT date, type, category, amount, description FROM transactions ORDER BY date");

    while (query.next()) {
        out << query.value(0).toString() << ","
            << query.value(1).toString() << ","
            << query.value(2).toString() << ","
            << query.value(3).toString() << ","
            << query.value(4).toString() << "\n";
    }

    file.close();
    QMessageBox::information(this, "Export Success", "Transactions exported successfully!");
}

void MainWindow::filterByCategory() { /* TODO: Implement filtering */ }
void MainWindow::filterByDateRange() { /* TODO: Implement date range filter */ }

// main.cpp
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
