#include "financetracker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QLocale>

FInanceTracker::FInanceTracker(QWidget *parent)
    : QMainWindow(parent), totalIncome(0), totalExpense(0)
{
    setupDatabase();
    setupUI();
    loadTransactions();
    updateSummary();
    updateChart();
}

FInanceTracker::~FInanceTracker()
{
    if (db.isOpen()) db.close();
}

void FInanceTracker::setupDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("finance.db");

    if (!db.open()) {
        QMessageBox::critical(this, "Database Error", db.lastError().text());
        return;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS transactions ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "date TEXT, type TEXT, category TEXT, amount REAL, description TEXT)");
}

QString FInanceTracker::formatRupiah(double amount) {
    QLocale idr(QLocale::Indonesian, QLocale::Indonesia);
    return idr.toCurrencyString(amount, "Rp");
}

void FInanceTracker::setupUI()
{
    setWindowTitle("Personal Finance Manager");
    resize(1100, 850);

    this->setStyleSheet(
        "QMainWindow { background-color: #121212; }"
        "QGroupBox { color: #ffffff; font-weight: bold; border: 1px solid #333; margin-top: 15px; padding: 10px; border-radius: 8px; }"
        "QLabel { color: #bbb; font-size: 10pt; }"
        "QLineEdit, QComboBox, QDateEdit { background-color: #1e1e1e; color: white; border: 1px solid #333; padding: 6px; border-radius: 4px; }"
        "QPushButton { background-color: #0078d4; color: white; border-radius: 4px; padding: 8px; font-weight: bold; }"
        "QPushButton:hover { background-color: #005a9e; }"
        "QTableWidget { background-color: #1e1e1e; color: white; gridline-color: #333; border-radius: 8px; }"
        "QHeaderView::section { background-color: #252525; color: white; padding: 5px; border: 1px solid #121212; }"
        );

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QGroupBox *summaryGroup = new QGroupBox("Financial Summary");
    QHBoxLayout *summaryLayout = new QHBoxLayout();

    totalIncomeLabel = new QLabel();
    totalExpenseLabel = new QLabel();
    balanceLabel = new QLabel();

    totalIncomeLabel->setStyleSheet("color: #4caf50; font-size: 15pt; font-weight: bold;");
    totalExpenseLabel->setStyleSheet("color: #f44336; font-size: 15pt; font-weight: bold;");
    balanceLabel->setStyleSheet("font-size: 15pt; font-weight: bold; padding: 8px; border-radius: 6px; color: white;");

    summaryLayout->addWidget(totalIncomeLabel);
    summaryLayout->addSpacing(30);
    summaryLayout->addWidget(totalExpenseLabel);
    summaryLayout->addStretch();
    summaryLayout->addWidget(balanceLabel);
    summaryGroup->setLayout(summaryLayout);
    mainLayout->addWidget(summaryGroup);

    QGroupBox *inputGroup = new QGroupBox("Add Transaction");
    QGridLayout *inputGrid = new QGridLayout();

    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    typeCombo = new QComboBox();
    typeCombo->addItems({"Expense", "Income"});
    categoryCombo = new QComboBox();
    categoryCombo->addItems({"Food", "Transport", "Bills", "Shopping", "Salary", "Investment", "Entertainment", "Other"});

    amountEdit = new QLineEdit();
    amountEdit->setPlaceholderText("Amount (Rp)");
    descriptionEdit = new QLineEdit();
    descriptionEdit->setPlaceholderText("Description (Optional)");

    addBtn = new QPushButton("Add Record");
    addBtn->setMinimumHeight(35);

    inputGrid->addWidget(new QLabel("Date"), 0, 0);
    inputGrid->addWidget(dateEdit, 1, 0);
    inputGrid->addWidget(new QLabel("Type"), 0, 1);
    inputGrid->addWidget(typeCombo, 1, 1);
    inputGrid->addWidget(new QLabel("Category"), 0, 2);
    inputGrid->addWidget(categoryCombo, 1, 2);
    inputGrid->addWidget(new QLabel("Amount"), 0, 3);
    inputGrid->addWidget(amountEdit, 1, 3);
    inputGrid->addWidget(new QLabel("Notes"), 0, 4);
    inputGrid->addWidget(descriptionEdit, 1, 4);
    inputGrid->addWidget(addBtn, 1, 5);

    inputGroup->setLayout(inputGrid);
    mainLayout->addWidget(inputGroup);

    // Chart
    pieChart = new QChart();
    pieChart->setAnimationOptions(QChart::SeriesAnimations);
    pieChart->setBackgroundVisible(false);
    pieChart->setTitleBrush(QBrush(Qt::white));

    chartView = new QChartView(pieChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setFixedHeight(280);
    mainLayout->addWidget(chartView);

    // Table
    transactionTable = new QTableWidget();
    transactionTable->setColumnCount(6);
    transactionTable->setHorizontalHeaderLabels({"ID", "Date", "Type", "Category", "Amount", "Description"});
    transactionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    transactionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    transactionTable->hideColumn(0);
    mainLayout->addWidget(transactionTable);

    // Actions
    QHBoxLayout *actionLayout = new QHBoxLayout();
    deleteBtn = new QPushButton("Delete Selected");
    deleteBtn->setStyleSheet("background-color: #d32f2f;");
    exportBtn = new QPushButton("Export CSV");

    actionLayout->addWidget(deleteBtn);
    actionLayout->addWidget(exportBtn);
    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    connect(addBtn, &QPushButton::clicked, this, &FInanceTracker::addTransaction);
    connect(deleteBtn, &QPushButton::clicked, this, &FInanceTracker::deleteTransaction);
    connect(exportBtn, &QPushButton::clicked, this, &FInanceTracker::exportToCSV);

    setCentralWidget(centralWidget);
}

void FInanceTracker::addTransaction() {
    QString date = dateEdit->date().toString("yyyy-MM-dd");
    QString type = typeCombo->currentText();
    QString category = categoryCombo->currentText();
    double amount = amountEdit->text().toDouble();
    QString desc = descriptionEdit->text();

    if (amount <= 0) {
        QMessageBox::warning(this, "Input Error", "Please enter a valid amount.");
        return;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO transactions (date, type, category, amount, description) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(date);
    query.addBindValue(type);
    query.addBindValue(category);
    query.addBindValue(amount);
    query.addBindValue(desc);

    if (query.exec()) {
        amountEdit->clear();
        descriptionEdit->clear();
        loadTransactions();
        updateSummary();
        updateChart();
    }
}

void FInanceTracker::deleteTransaction() {
    int row = transactionTable->currentRow();
    if (row < 0) return;

    int id = transactionTable->item(row, 0)->text().toInt();
    QSqlQuery query;
    query.prepare("DELETE FROM transactions WHERE id = ?");
    query.addBindValue(id);

    if (query.exec()) {
        loadTransactions();
        updateSummary();
        updateChart();
    }
}

void FInanceTracker::loadTransactions() {
    transactionTable->setRowCount(0);
    QSqlQuery query("SELECT * FROM transactions ORDER BY date DESC");
    while (query.next()) {
        int row = transactionTable->rowCount();
        transactionTable->insertRow(row);
        for(int i=0; i<6; ++i) {
            QString val = (i == 4) ? formatRupiah(query.value(i).toDouble()) : query.value(i).toString();
            transactionTable->setItem(row, i, new QTableWidgetItem(val));
        }
    }
}

void FInanceTracker::updateSummary() {
    totalIncome = 0; totalExpense = 0;
    QSqlQuery query("SELECT type, SUM(amount) FROM transactions GROUP BY type");
    while (query.next()) {
        if (query.value(0).toString() == "Income") totalIncome = query.value(1).toDouble();
        else totalExpense = query.value(1).toDouble();
    }

    double balance = totalIncome - totalExpense;
    totalIncomeLabel->setText("Income: " + formatRupiah(totalIncome));
    totalExpenseLabel->setText("Expenses: " + formatRupiah(totalExpense));
    balanceLabel->setText("Balance: " + formatRupiah(balance));
    balanceLabel->setStyleSheet(QString("background-color: %1; font-weight: bold; font-size: 15pt; border-radius: 6px; padding: 8px;")
                                    .arg(balance >= 0 ? "#2e7d32" : "#c62828"));
}

void FInanceTracker::updateChart() {
    pieChart->removeAllSeries();
    QPieSeries *series = new QPieSeries();
    QSqlQuery query("SELECT category, SUM(amount) FROM transactions WHERE type='Expense' GROUP BY category");
    while (query.next()) {
        series->append(query.value(0).toString() + " (" + formatRupiah(query.value(1).toDouble()) + ")", query.value(1).toDouble());
    }
    pieChart->addSeries(series);
}

void FInanceTracker::exportToCSV() {
    QString filename = QFileDialog::getSaveFileName(this, "Export", "", "CSV Files (*.csv)");
    if (filename.isEmpty()) return;

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Date,Type,Category,Amount,Description\n";
        QSqlQuery query("SELECT date, type, category, amount, description FROM transactions");
        while (query.next()) {
            out << query.value(0).toString() << "," << query.value(1).toString() << ","
                << query.value(2).toString() << "," << query.value(3).toString() << "," << query.value(4).toString() << "\n";
        }
        file.close();
        QMessageBox::information(this, "Success", "Data exported to " + filename);
    }
}

void FInanceTracker::filterByCategory() { /* TODO: Implement filtering */ }
void FInanceTracker::filterByDateRange() { /* TODO: Implement date range filter */ }
