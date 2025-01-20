#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QRegularExpression>
#include <QString>
#include <QList>
#include <QPair>
#include <QMessageBox>
#include <QMap>

// Объявление пространства имен Ui
namespace Ui {
class MainWindow; // Форвард-декларация класса MainWindow
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void analyzeText(); // Лексический анализ
    void openFile();    // Открытие файла (лексический анализ)
    void analyzeTextSyntax(); // Синтаксический анализ
    void openFileSyntax();    // Открытие файла (синтаксический анализ)

private:
    Ui::MainWindow *ui; // Указатель на интерфейс
    bool errorFlag; // Флаг для отслеживания ошибки
    QMap<QString, QString> variables; // Хранение значений переменных

    // Методы для работы с лексемами
    QList<QPair<QString, QString>> tokenize(const QString &text);
    void addLexemeToTable(const QString &expression, const QString &lexeme, const QString &type, QTableWidget *table);

    // Методы для синтаксического анализа
    bool parseProgram(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error);
    bool parseExpressionList(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error);
    bool parseExpression(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error);
    bool parseAssignment(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error);
    bool parseLogicalExpression(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error);
    bool parseSubLogical(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error);
    bool parseSubSubLogical(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error);
    bool parsePrimaryExpression(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error);

    // Вспомогательные методы
    void displaySyntaxResult(const QString &result); // Вывод результата синтаксического анализа
    void resetTextEditStyle(); // Сброс стиля текстового поля
};

#endif // MAINWINDOW_H
