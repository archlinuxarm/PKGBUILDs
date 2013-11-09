/********************************************************************************
** Form generated from reading UI file 'settings_general.ui'
**
** Created by: Qt User Interface Compiler version 4.8.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGS_GENERAL_H
#define UI_SETTINGS_GENERAL_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GeneralConfigPage
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QRadioButton *startNewSessionOption;
    QRadioButton *startLastSessionOption;
    QHBoxLayout *horizontalLayout;
    QRadioButton *startNamedSessionOption;
    QLineEdit *startSessionName;
    QSpacerItem *verticalSpacer;
    QButtonGroup *startSessionOptions;

    void setupUi(QWidget *GeneralConfigPage)
    {
        if (GeneralConfigPage->objectName().isEmpty())
            GeneralConfigPage->setObjectName(QString::fromUtf8("GeneralConfigPage"));
        GeneralConfigPage->resize(500, 356);
        verticalLayout = new QVBoxLayout(GeneralConfigPage);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label = new QLabel(GeneralConfigPage);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout->addWidget(label);

        startNewSessionOption = new QRadioButton(GeneralConfigPage);
        startSessionOptions = new QButtonGroup(GeneralConfigPage);
        startSessionOptions->setObjectName(QString::fromUtf8("startSessionOptions"));
        startSessionOptions->addButton(startNewSessionOption);
        startNewSessionOption->setObjectName(QString::fromUtf8("startNewSessionOption"));

        verticalLayout->addWidget(startNewSessionOption);

        startLastSessionOption = new QRadioButton(GeneralConfigPage);
        startSessionOptions->addButton(startLastSessionOption);
        startLastSessionOption->setObjectName(QString::fromUtf8("startLastSessionOption"));

        verticalLayout->addWidget(startLastSessionOption);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        startNamedSessionOption = new QRadioButton(GeneralConfigPage);
        startSessionOptions->addButton(startNamedSessionOption);
        startNamedSessionOption->setObjectName(QString::fromUtf8("startNamedSessionOption"));

        horizontalLayout->addWidget(startNamedSessionOption);

        startSessionName = new QLineEdit(GeneralConfigPage);
        startSessionName->setObjectName(QString::fromUtf8("startSessionName"));

        horizontalLayout->addWidget(startSessionName);


        verticalLayout->addLayout(horizontalLayout);

        verticalSpacer = new QSpacerItem(20, 250, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(GeneralConfigPage);

        QMetaObject::connectSlotsByName(GeneralConfigPage);
    } // setupUi

    void retranslateUi(QWidget *GeneralConfigPage)
    {
        GeneralConfigPage->setWindowTitle(QApplication::translate("GeneralConfigPage", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("GeneralConfigPage", "On application start:", 0, QApplication::UnicodeUTF8));
        startNewSessionOption->setText(QApplication::translate("GeneralConfigPage", "Start a new session", 0, QApplication::UnicodeUTF8));
        startLastSessionOption->setText(QApplication::translate("GeneralConfigPage", "Load last session", 0, QApplication::UnicodeUTF8));
        startNamedSessionOption->setText(QApplication::translate("GeneralConfigPage", "Load session:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class GeneralConfigPage: public Ui_GeneralConfigPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGS_GENERAL_H
