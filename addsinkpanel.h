//
// Created by SuperTuxi on 11.06.25.
//

#ifndef ADDSINKPANEL_H
#define ADDSINKPANEL_H

#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>

class MainWindow;

class AddSinkPanel : public QWidget {
    Q_OBJECT
public:
    explicit AddSinkPanel(MainWindow* mainWindow);

    [[nodiscard]] MainWindow* main_window() const {
        return mainWindow;
    }
private:
    MainWindow* mainWindow;

    QGridLayout layout;
    QLineEdit nameBox;
    QPushButton confirmButton;

    void onNameBoxReturnPressed();
    void onConfirmButtonClicked();
};

#endif //ADDSINKPANEL_H
