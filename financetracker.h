#ifndef FINANCETRACKER_H
#define FINANCETRACKER_H

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

QT_BEGIN_NAMESPACE
namespace Ui {
class FInanceTracker;
}
QT_END_NAMESPACE

class FInanceTracker : public QMainWindow
{
    Q_OBJECT

public:
    FInanceTracker(QWidget *parent = nullptr);
    ~FInanceTracker();

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
    QString formatRupiah(double amount);

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

#endif // FINANCETRACKER_H
