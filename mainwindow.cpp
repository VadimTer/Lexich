#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QRegularExpression>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), errorFlag(false) {
    ui->setupUi(this);

    // Подключение кнопок к слотам (лексический анализ)
    connect(ui->analyzeButton, &QPushButton::clicked, this, &MainWindow::analyzeText);
    connect(ui->openFileButton, &QPushButton::clicked, this, &MainWindow::openFile);

    // Подключение кнопок к слотам (синтаксический анализ)
    connect(ui->analyzeButtonSyntax, &QPushButton::clicked, this, &MainWindow::analyzeTextSyntax);
    connect(ui->openFileButtonSyntax, &QPushButton::clicked, this, &MainWindow::openFileSyntax);

    // Уменьшаем высоту окна для вывода результата синтаксического анализа
    ui->syntaxResultText->setMaximumHeight(ui->syntaxResultText->height() / 4);

    // Растягиваем таблицу лексем вертикально
    ui->lexicalTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->syntaxTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Увеличиваем высоту строки для вывода результата
    ui->syntaxResultText->setMinimumHeight(50); // Увеличиваем высоту для 1-2 строк текста
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::analyzeText() {
    // Очистка таблицы перед новым анализом
    ui->lexicalTable->setRowCount(0);

    // Получение текста из текстового поля
    QString text = ui->inputTextEdit->toPlainText();

    // Лексический анализ
    QList<QPair<QString, QString>> tokens = tokenize(text);
    for (const auto &token : tokens) {
        addLexemeToTable("", token.first, token.second, ui->lexicalTable);
    }
}

void MainWindow::openFile() {
    // Открытие диалога выбора файла
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");
    if (fileName.isEmpty()) {
        return; // Если файл не выбран, выходим
    }

    // Чтение файла
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл.");
        return;
    }

    QTextStream in(&file);
    ui->inputTextEdit->setText(in.readAll()); // Загрузка текста в QTextEdit
    file.close();
}

void MainWindow::analyzeTextSyntax() {
    // Очистка таблицы и поля для результатов синтаксического анализа
    ui->syntaxTable->setRowCount(0);
    ui->syntaxResultText->clear();

    // Получение текста из текстового поля
    QString text = ui->inputTextEditSyntax->toPlainText();

    // Лексический анализ
    QList<QPair<QString, QString>> tokens = tokenize(text);

    // Если лексический анализ завершился с ошибкой, выходим
    if (errorFlag) {
        displaySyntaxResult("Ошибка лексического анализа. Исправьте текст и попробуйте снова.");
        QMessageBox::critical(this, "Ошибка", "Ошибка лексического анализа. Исправьте текст и попробуйте снова.");
        return;
    }

    // Синтаксический анализ
    int pos = 0;
    QString error;
    if (parseProgram(tokens, pos, error)) {
        // Добавляем лексемы в таблицу
        QString currentExpression;
        int expressionCounter = 1; // Счетчик выражений
        for (int i = 0; i < tokens.size(); ++i) {
            if (tokens[i].first == ";") {
                // Добавляем разделитель в таблицу
                addLexemeToTable(currentExpression, tokens[i].first, tokens[i].second, ui->syntaxTable);
                currentExpression.clear();
                expressionCounter++;
            } else {
                if (currentExpression.isEmpty()) {
                    currentExpression = "Выражение " + QString::number(expressionCounter);
                }
                addLexemeToTable(currentExpression, tokens[i].first, tokens[i].second, ui->syntaxTable);
            }
        }
        displaySyntaxResult("Синтаксический анализ завершен успешно.");
    } else {
        displaySyntaxResult("Ошибка: " + error);
        QMessageBox::critical(this, "Ошибка", "Ошибка синтаксического анализа: " + error);
    }
}

void MainWindow::openFileSyntax() {
    // Открытие диалога выбора файла
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");
    if (fileName.isEmpty()) {
        return; // Если файл не выбран, выходим
    }

    // Чтение файла
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл.");
        return;
    }

    QTextStream in(&file);
    ui->inputTextEditSyntax->setText(in.readAll()); // Загрузка текста в QTextEdit
    file.close();
}

QList<QPair<QString, QString>> MainWindow::tokenize(const QString &text) {
    QList<QPair<QString, QString>> tokens;
    errorFlag = false; // Сбрасываем флаг ошибки перед анализом

    // Регулярные выражения для поиска лексем
    QRegularExpression identifierRegex("\\b[a-zA-Z][a-zA-Z0-9]*\\b");
    QRegularExpression constantRegex("\\b[TF]\\b");
    QRegularExpression operatorRegex(":=|\\bor\\b|\\bxor\\b|\\band\\b|\\bnot\\b");
    QRegularExpression parenthesisRegex("[()]");
    QRegularExpression delimiterRegex(";");
    QRegularExpression commentRegex("//.*?//");

    int pos = 0;
    while (pos < text.length()) {
        if (text.at(pos).isSpace()) {
            pos++;
            continue;
        }

        QRegularExpressionMatch commentMatch = commentRegex.match(text, pos);
        if (commentMatch.hasMatch() && commentMatch.capturedStart() == pos) {
            pos = commentMatch.capturedEnd();
            continue;
        }

        bool matchFound = false;
        QList<QPair<QRegularExpression, QString>> regexList = {
            {constantRegex, "Константа"},
            {operatorRegex, "Оператор"},
            {identifierRegex, "Идентификатор"},
            {parenthesisRegex, "Скобка"},
            {delimiterRegex, "Разделитель"}
        };

        for (const auto &regexPair : regexList) {
            QRegularExpressionMatch match = regexPair.first.match(text, pos);
            if (match.hasMatch() && match.capturedStart() == pos) {
                tokens.append({match.captured(), regexPair.second});
                pos = match.capturedEnd();
                matchFound = true;
                break;
            }
        }

        if (!matchFound) {
            // Если лексема не найдена, это ошибка
            QTextEdit *inputTextEdit = (ui->tabWidget->currentIndex() == 0) ? ui->inputTextEdit : ui->inputTextEditSyntax;
            inputTextEdit->setStyleSheet("QTextEdit { background-color: #FFCCCC; }"); // Подсветка ошибки
            QTextCursor cursor = inputTextEdit->textCursor();
            cursor.setPosition(pos);
            inputTextEdit->setTextCursor(cursor);
            QMessageBox::critical(this, "Ошибка", QString("Неизвестный символ: '") + text.at(pos) + "'");
            errorFlag = true; // Устанавливаем флаг ошибки
            return tokens;
        }
    }

    // Если ошибок не было, сбрасываем стиль
    resetTextEditStyle();
    errorFlag = false; // Сбрасываем флаг ошибки

    return tokens;
}

void MainWindow::addLexemeToTable(const QString &expression, const QString &lexeme, const QString &type, QTableWidget *table) {
    int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem(expression));
    table->setItem(row, 1, new QTableWidgetItem(lexeme));
    table->setItem(row, 2, new QTableWidgetItem(type));
}

bool MainWindow::parseProgram(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error) {
    return parseExpressionList(tokens, pos, error);
}

bool MainWindow::parseExpressionList(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error) {
    while (pos < tokens.size()) {
        if (!parseExpression(tokens, pos, error)) {
            return false;
        }
        if (pos < tokens.size() && tokens[pos].first == ";") {
            pos++;
        } else if (pos < tokens.size()) {
            error = "Ожидается ';'";
            return false;
        }
    }
    return true;
}

bool MainWindow::parseExpression(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error) {
    if (pos >= tokens.size()) {
        error = "Неожиданный конец выражения";
        return false;
    }

    // Если это присваивание
    if (tokens[pos].second == "Идентификатор" && pos + 1 < tokens.size() && tokens[pos + 1].first == ":=") {
        return parseAssignment(tokens, pos, error);
    }
    // Если это логическое выражение
    else {
        // Проверяем, что выражение не состоит только из одного идентификатора или константы
        if (tokens[pos].second == "Идентификатор" || tokens[pos].second == "Константа") {
            if (pos + 1 < tokens.size() && tokens[pos + 1].first == ";") {
                error = "Логическое выражение не может состоять только из идентификатора или константы";
                return false;
            }
        }
        return parseLogicalExpression(tokens, pos, error);
    }
}

bool MainWindow::parseAssignment(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error) {
    if (tokens[pos].second != "Идентификатор") {
        error = "Ожидается идентификатор";
        return false;
    }
    QString variable = tokens[pos].first; // Запоминаем имя переменной
    pos++;
    if (pos >= tokens.size() || tokens[pos].first != ":=") {
        error = "Ожидается ':='";
        return false;
    }
    pos++;
    if (!parseLogicalExpression(tokens, pos, error)) {
        return false;
    }
    // Запоминаем значение переменной
    variables[variable] = "T"; // Пример: присваиваем значение "T"
    return true;
}

bool MainWindow::parseLogicalExpression(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error) {
    if (!parseSubLogical(tokens, pos, error)) {
        return false;
    }

    // Проверяем, что логическое выражение содержит хотя бы один оператор
    bool hasOperator = false;
    int initialPos = pos;

    while (pos < tokens.size() && (tokens[pos].first == "or" || tokens[pos].first == "xor" || tokens[pos].first == "and")) {
        hasOperator = true;
        pos++;
        if (!parseLogicalExpression(tokens, pos, error)) {
            return false;
        }
    }

    // Если операторов нет, это ошибка
    if (!hasOperator && initialPos != pos) {
        error = "Логическое выражение должно содержать хотя бы один оператор (and, or, xor)";
        return false;
    }

    return true;
}

bool MainWindow::parseSubLogical(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error) {
    if (!parseSubSubLogical(tokens, pos, error)) {
        return false;
    }
    while (pos < tokens.size() && tokens[pos].first == "xor") {
        pos++;
        if (!parseSubLogical(tokens, pos, error)) {
            return false;
        }
    }
    return true;
}

bool MainWindow::parseSubSubLogical(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error) {
    if (!parsePrimaryExpression(tokens, pos, error)) {
        return false;
    }
    while (pos < tokens.size() && tokens[pos].first == "and") {
        pos++;
        if (!parseSubSubLogical(tokens, pos, error)) {
            return false;
        }
    }
    return true;
}

bool MainWindow::parsePrimaryExpression(const QList<QPair<QString, QString>> &tokens, int &pos, QString &error) {
    if (pos >= tokens.size()) {
        error = "Неожиданный конец выражения";
        return false;
    }
    if (tokens[pos].second == "Идентификатор") {
        // Проверяем, была ли переменной присвоено значение
        if (!variables.contains(tokens[pos].first)) {
            error = "Переменная '" + tokens[pos].first + "' не была инициализирована";
            return false;
        }
        pos++;
        return true;
    } else if (tokens[pos].second == "Константа") {
        pos++;
        return true;
    } else if (tokens[pos].first == "not") {
        pos++;
        return parsePrimaryExpression(tokens, pos, error);
    } else if (tokens[pos].first == "(") {
        pos++;
        if (!parseLogicalExpression(tokens, pos, error)) {
            return false;
        }
        if (pos >= tokens.size() || tokens[pos].first != ")") {
            error = "Ожидается ')'";
            return false;
        }
        pos++;
        return true;
    } else {
        error = "Ожидается идентификатор, константа или выражение в скобках";
        return false;
    }
}

void MainWindow::displaySyntaxResult(const QString &result) {
    ui->syntaxResultText->setPlainText(result);
}

void MainWindow::resetTextEditStyle() {
    QTextEdit *inputTextEdit = (ui->tabWidget->currentIndex() == 0) ? ui->inputTextEdit : ui->inputTextEditSyntax;
    inputTextEdit->setStyleSheet(""); // Сбрасываем стиль
}
