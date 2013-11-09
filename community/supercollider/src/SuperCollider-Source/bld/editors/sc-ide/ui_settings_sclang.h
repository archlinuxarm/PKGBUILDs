/********************************************************************************
** Form generated from reading UI file 'settings_sclang.ui'
**
** Created by: Qt User Interface Compiler version 4.8.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGS_SCLANG_H
#define UI_SETTINGS_SCLANG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QFormLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "path_chooser_widget.hpp"

QT_BEGIN_NAMESPACE

class Ui_SclangConfigPage
{
public:
    QVBoxLayout *verticalLayout;
    QWidget *widget;
    QFormLayout *formLayout;
    QCheckBox *autoStart;
    QLabel *label_3;
    ScIDE::PathChooserWidget *runtimeDir;
    QGroupBox *ide_sclang_override_configuration_file;
    QFormLayout *formLayout_2;
    QLabel *label;
    QWidget *widget_3;
    QGridLayout *gridLayout;
    QListWidget *sclang_include_directories;
    QVBoxLayout *verticalLayout_3;
    QToolButton *sclang_add_include;
    QToolButton *sclang_remove_include;
    QLabel *excludeLabel;
    QWidget *widget_2;
    QGridLayout *gridLayout_2;
    QListWidget *sclang_exclude_directories;
    QVBoxLayout *verticalLayout_2;
    QToolButton *sclang_add_exclude;
    QToolButton *sclang_remove_exclude;
    QCheckBox *sclang_post_inline_warnings;

    void setupUi(QWidget *SclangConfigPage)
    {
        if (SclangConfigPage->objectName().isEmpty())
            SclangConfigPage->setObjectName(QString::fromUtf8("SclangConfigPage"));
        SclangConfigPage->resize(613, 421);
        verticalLayout = new QVBoxLayout(SclangConfigPage);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        widget = new QWidget(SclangConfigPage);
        widget->setObjectName(QString::fromUtf8("widget"));
        formLayout = new QFormLayout(widget);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        autoStart = new QCheckBox(widget);
        autoStart->setObjectName(QString::fromUtf8("autoStart"));

        formLayout->setWidget(0, QFormLayout::FieldRole, autoStart);

        label_3 = new QLabel(widget);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_3);

        runtimeDir = new ScIDE::PathChooserWidget(widget);
        runtimeDir->setObjectName(QString::fromUtf8("runtimeDir"));

        formLayout->setWidget(1, QFormLayout::FieldRole, runtimeDir);


        verticalLayout->addWidget(widget);

        ide_sclang_override_configuration_file = new QGroupBox(SclangConfigPage);
        ide_sclang_override_configuration_file->setObjectName(QString::fromUtf8("ide_sclang_override_configuration_file"));
        ide_sclang_override_configuration_file->setEnabled(true);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ide_sclang_override_configuration_file->sizePolicy().hasHeightForWidth());
        ide_sclang_override_configuration_file->setSizePolicy(sizePolicy);
        ide_sclang_override_configuration_file->setFlat(false);
        ide_sclang_override_configuration_file->setCheckable(false);
        ide_sclang_override_configuration_file->setChecked(false);
        formLayout_2 = new QFormLayout(ide_sclang_override_configuration_file);
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        formLayout_2->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        label = new QLabel(ide_sclang_override_configuration_file);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label);

        widget_3 = new QWidget(ide_sclang_override_configuration_file);
        widget_3->setObjectName(QString::fromUtf8("widget_3"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(widget_3->sizePolicy().hasHeightForWidth());
        widget_3->setSizePolicy(sizePolicy1);
        gridLayout = new QGridLayout(widget_3);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        sclang_include_directories = new QListWidget(widget_3);
        sclang_include_directories->setObjectName(QString::fromUtf8("sclang_include_directories"));

        gridLayout->addWidget(sclang_include_directories, 0, 0, 2, 1);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        sclang_add_include = new QToolButton(widget_3);
        sclang_add_include->setObjectName(QString::fromUtf8("sclang_add_include"));
        sclang_add_include->setEnabled(true);

        verticalLayout_3->addWidget(sclang_add_include);

        sclang_remove_include = new QToolButton(widget_3);
        sclang_remove_include->setObjectName(QString::fromUtf8("sclang_remove_include"));
        sclang_remove_include->setEnabled(true);

        verticalLayout_3->addWidget(sclang_remove_include);


        gridLayout->addLayout(verticalLayout_3, 0, 2, 1, 1);


        formLayout_2->setWidget(0, QFormLayout::FieldRole, widget_3);

        excludeLabel = new QLabel(ide_sclang_override_configuration_file);
        excludeLabel->setObjectName(QString::fromUtf8("excludeLabel"));

        formLayout_2->setWidget(1, QFormLayout::LabelRole, excludeLabel);

        widget_2 = new QWidget(ide_sclang_override_configuration_file);
        widget_2->setObjectName(QString::fromUtf8("widget_2"));
        sizePolicy1.setHeightForWidth(widget_2->sizePolicy().hasHeightForWidth());
        widget_2->setSizePolicy(sizePolicy1);
        gridLayout_2 = new QGridLayout(widget_2);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        sclang_exclude_directories = new QListWidget(widget_2);
        sclang_exclude_directories->setObjectName(QString::fromUtf8("sclang_exclude_directories"));

        gridLayout_2->addWidget(sclang_exclude_directories, 0, 1, 3, 1);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        sclang_add_exclude = new QToolButton(widget_2);
        sclang_add_exclude->setObjectName(QString::fromUtf8("sclang_add_exclude"));
        sclang_add_exclude->setEnabled(true);

        verticalLayout_2->addWidget(sclang_add_exclude);

        sclang_remove_exclude = new QToolButton(widget_2);
        sclang_remove_exclude->setObjectName(QString::fromUtf8("sclang_remove_exclude"));
        sclang_remove_exclude->setEnabled(true);

        verticalLayout_2->addWidget(sclang_remove_exclude);


        gridLayout_2->addLayout(verticalLayout_2, 0, 3, 1, 1);


        formLayout_2->setWidget(1, QFormLayout::FieldRole, widget_2);

        sclang_post_inline_warnings = new QCheckBox(ide_sclang_override_configuration_file);
        sclang_post_inline_warnings->setObjectName(QString::fromUtf8("sclang_post_inline_warnings"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, sclang_post_inline_warnings);


        verticalLayout->addWidget(ide_sclang_override_configuration_file);

        ide_sclang_override_configuration_file->raise();
        widget->raise();

        retranslateUi(SclangConfigPage);

        QMetaObject::connectSlotsByName(SclangConfigPage);
    } // setupUi

    void retranslateUi(QWidget *SclangConfigPage)
    {
        SclangConfigPage->setWindowTitle(QApplication::translate("SclangConfigPage", "Form", 0, QApplication::UnicodeUTF8));
        autoStart->setText(QApplication::translate("SclangConfigPage", "Start Interpreter With IDE", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("SclangConfigPage", "Runtime Directory:", 0, QApplication::UnicodeUTF8));
        ide_sclang_override_configuration_file->setTitle(QApplication::translate("SclangConfigPage", "Interpreter Options", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("SclangConfigPage", "Include:", 0, QApplication::UnicodeUTF8));
        sclang_add_include->setText(QApplication::translate("SclangConfigPage", "+", 0, QApplication::UnicodeUTF8));
        sclang_remove_include->setText(QApplication::translate("SclangConfigPage", "-", 0, QApplication::UnicodeUTF8));
        excludeLabel->setText(QApplication::translate("SclangConfigPage", "Exclude:", 0, QApplication::UnicodeUTF8));
        sclang_add_exclude->setText(QApplication::translate("SclangConfigPage", "+", 0, QApplication::UnicodeUTF8));
        sclang_remove_exclude->setText(QApplication::translate("SclangConfigPage", "-", 0, QApplication::UnicodeUTF8));
        sclang_post_inline_warnings->setText(QApplication::translate("SclangConfigPage", "Post Inline Warnings", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class SclangConfigPage: public Ui_SclangConfigPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGS_SCLANG_H
