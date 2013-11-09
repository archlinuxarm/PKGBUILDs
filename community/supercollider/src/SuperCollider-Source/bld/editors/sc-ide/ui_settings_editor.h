/********************************************************************************
** Form generated from reading UI file 'settings_editor.ui'
**
** Created by: Qt User Interface Compiler version 4.8.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGS_EDITOR_H
#define UI_SETTINGS_EDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QSplitter>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>
#include <QtGui/QTreeWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "color_widget.hpp"

QT_BEGIN_NAMESPACE

class Ui_EditorConfigPage
{
public:
    QVBoxLayout *verticalLayout;
    QSplitter *splitter;
    QTabWidget *tabs;
    QWidget *tab;
    QVBoxLayout *verticalLayout_4;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpinBox *indentWidth;
    QSpacerItem *horizontalSpacer;
    QCheckBox *spaceIndent;
    QGroupBox *groupBox_4;
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_11;
    QSpinBox *blinkDuration;
    QSpacerItem *horizontalSpacer_2;
    QCheckBox *stepForwardEvaluation;
    QSpacerItem *verticalSpacer;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout_6;
    QCheckBox *editorLineWrap;
    QCheckBox *postWindowLineWrap;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_12;
    QSpinBox *postWindowScrollback;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *verticalSpacer_2;
    QWidget *colorsTab;
    QVBoxLayout *verticalLayout_8;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QComboBox *fontCombo;
    QSpinBox *fontSize;
    QCheckBox *onlyMonoFonts;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_16;
    QLabel *label_15;
    QSpacerItem *horizontalSpacer_6;
    QCheckBox *fontAntialias;
    QSpacerItem *horizontalSpacer_5;
    QGroupBox *groupBox_2;
    QHBoxLayout *horizontalLayout_5;
    QTreeWidget *textFormats;
    QWidget *textFormatEdit;
    QVBoxLayout *verticalLayout_2;
    QGridLayout *gridLayout;
    QLabel *label_13;
    ScIDE::ColorWidget *bgPicker;
    QToolButton *bgClearBtn;
    QLabel *label_14;
    ScIDE::ColorWidget *fgPicker;
    QToolButton *fgClearBtn;
    QVBoxLayout *verticalLayout_7;
    QCheckBox *italicOption;
    QCheckBox *boldOption;
    QSpacerItem *verticalSpacer_3;

    void setupUi(QWidget *EditorConfigPage)
    {
        if (EditorConfigPage->objectName().isEmpty())
            EditorConfigPage->setObjectName(QString::fromUtf8("EditorConfigPage"));
        EditorConfigPage->resize(615, 471);
        verticalLayout = new QVBoxLayout(EditorConfigPage);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        splitter = new QSplitter(EditorConfigPage);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(Qt::Vertical);
        tabs = new QTabWidget(splitter);
        tabs->setObjectName(QString::fromUtf8("tabs"));
        tabs->setDocumentMode(false);
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        verticalLayout_4 = new QVBoxLayout(tab);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        groupBox_3 = new QGroupBox(tab);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        verticalLayout_3 = new QVBoxLayout(groupBox_3);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(groupBox_3);
        label->setObjectName(QString::fromUtf8("label"));
        label->setIndent(4);

        horizontalLayout->addWidget(label);

        indentWidth = new QSpinBox(groupBox_3);
        indentWidth->setObjectName(QString::fromUtf8("indentWidth"));
        indentWidth->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        indentWidth->setMinimum(1);
        indentWidth->setValue(4);

        horizontalLayout->addWidget(indentWidth);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout_3->addLayout(horizontalLayout);

        spaceIndent = new QCheckBox(groupBox_3);
        spaceIndent->setObjectName(QString::fromUtf8("spaceIndent"));

        verticalLayout_3->addWidget(spaceIndent);


        verticalLayout_4->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(tab);
        groupBox_4->setObjectName(QString::fromUtf8("groupBox_4"));
        verticalLayout_5 = new QVBoxLayout(groupBox_4);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_11 = new QLabel(groupBox_4);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setIndent(4);

        horizontalLayout_4->addWidget(label_11);

        blinkDuration = new QSpinBox(groupBox_4);
        blinkDuration->setObjectName(QString::fromUtf8("blinkDuration"));
        blinkDuration->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        blinkDuration->setMinimum(1);
        blinkDuration->setMaximum(1000);
        blinkDuration->setValue(600);

        horizontalLayout_4->addWidget(blinkDuration);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_2);


        verticalLayout_5->addLayout(horizontalLayout_4);

        stepForwardEvaluation = new QCheckBox(groupBox_4);
        stepForwardEvaluation->setObjectName(QString::fromUtf8("stepForwardEvaluation"));

        verticalLayout_5->addWidget(stepForwardEvaluation);


        verticalLayout_4->addWidget(groupBox_4);

        verticalSpacer = new QSpacerItem(20, 248, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer);

        tabs->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        verticalLayout_6 = new QVBoxLayout(tab_2);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        editorLineWrap = new QCheckBox(tab_2);
        editorLineWrap->setObjectName(QString::fromUtf8("editorLineWrap"));

        verticalLayout_6->addWidget(editorLineWrap);

        postWindowLineWrap = new QCheckBox(tab_2);
        postWindowLineWrap->setObjectName(QString::fromUtf8("postWindowLineWrap"));

        verticalLayout_6->addWidget(postWindowLineWrap);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label_12 = new QLabel(tab_2);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setIndent(4);

        horizontalLayout_3->addWidget(label_12);

        postWindowScrollback = new QSpinBox(tab_2);
        postWindowScrollback->setObjectName(QString::fromUtf8("postWindowScrollback"));
        postWindowScrollback->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        postWindowScrollback->setMinimum(0);
        postWindowScrollback->setMaximum(1000000);
        postWindowScrollback->setSingleStep(1);
        postWindowScrollback->setValue(1000);

        horizontalLayout_3->addWidget(postWindowScrollback);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);


        verticalLayout_6->addLayout(horizontalLayout_3);

        verticalSpacer_2 = new QSpacerItem(20, 346, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_2);

        tabs->addTab(tab_2, QString());
        colorsTab = new QWidget();
        colorsTab->setObjectName(QString::fromUtf8("colorsTab"));
        verticalLayout_8 = new QVBoxLayout(colorsTab);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        groupBox = new QGroupBox(colorsTab);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        fontCombo = new QComboBox(groupBox);
        fontCombo->setObjectName(QString::fromUtf8("fontCombo"));
        fontCombo->setFrame(true);

        gridLayout_2->addWidget(fontCombo, 1, 1, 1, 1);

        fontSize = new QSpinBox(groupBox);
        fontSize->setObjectName(QString::fromUtf8("fontSize"));
        fontSize->setMinimum(1);

        gridLayout_2->addWidget(fontSize, 1, 4, 1, 1);

        onlyMonoFonts = new QCheckBox(groupBox);
        onlyMonoFonts->setObjectName(QString::fromUtf8("onlyMonoFonts"));

        gridLayout_2->addWidget(onlyMonoFonts, 0, 0, 1, 2);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_4, 1, 2, 1, 1);

        label_16 = new QLabel(groupBox);
        label_16->setObjectName(QString::fromUtf8("label_16"));

        gridLayout_2->addWidget(label_16, 1, 0, 1, 1);

        label_15 = new QLabel(groupBox);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        gridLayout_2->addWidget(label_15, 1, 3, 1, 1);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_6, 1, 7, 1, 1);

        fontAntialias = new QCheckBox(groupBox);
        fontAntialias->setObjectName(QString::fromUtf8("fontAntialias"));
        fontAntialias->setEnabled(true);
        fontAntialias->setChecked(true);

        gridLayout_2->addWidget(fontAntialias, 1, 6, 1, 1);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_5, 1, 5, 1, 1);


        verticalLayout_8->addWidget(groupBox);

        groupBox_2 = new QGroupBox(colorsTab);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        horizontalLayout_5 = new QHBoxLayout(groupBox_2);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        textFormats = new QTreeWidget(groupBox_2);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        textFormats->setHeaderItem(__qtreewidgetitem);
        textFormats->setObjectName(QString::fromUtf8("textFormats"));
        textFormats->setRootIsDecorated(false);
        textFormats->setHeaderHidden(true);
        textFormats->header()->setVisible(false);

        horizontalLayout_5->addWidget(textFormats);

        textFormatEdit = new QWidget(groupBox_2);
        textFormatEdit->setObjectName(QString::fromUtf8("textFormatEdit"));
        verticalLayout_2 = new QVBoxLayout(textFormatEdit);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_13 = new QLabel(textFormatEdit);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        gridLayout->addWidget(label_13, 0, 0, 1, 1);

        bgPicker = new ScIDE::ColorWidget(textFormatEdit);
        bgPicker->setObjectName(QString::fromUtf8("bgPicker"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(bgPicker->sizePolicy().hasHeightForWidth());
        bgPicker->setSizePolicy(sizePolicy);
        bgPicker->setMinimumSize(QSize(25, 25));

        gridLayout->addWidget(bgPicker, 0, 1, 1, 1);

        bgClearBtn = new QToolButton(textFormatEdit);
        bgClearBtn->setObjectName(QString::fromUtf8("bgClearBtn"));
        bgClearBtn->setMinimumSize(QSize(25, 25));

        gridLayout->addWidget(bgClearBtn, 0, 2, 1, 1);

        label_14 = new QLabel(textFormatEdit);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        gridLayout->addWidget(label_14, 1, 0, 1, 1);

        fgPicker = new ScIDE::ColorWidget(textFormatEdit);
        fgPicker->setObjectName(QString::fromUtf8("fgPicker"));
        sizePolicy.setHeightForWidth(fgPicker->sizePolicy().hasHeightForWidth());
        fgPicker->setSizePolicy(sizePolicy);
        fgPicker->setMinimumSize(QSize(25, 25));

        gridLayout->addWidget(fgPicker, 1, 1, 1, 1);

        fgClearBtn = new QToolButton(textFormatEdit);
        fgClearBtn->setObjectName(QString::fromUtf8("fgClearBtn"));
        fgClearBtn->setMinimumSize(QSize(25, 25));

        gridLayout->addWidget(fgClearBtn, 1, 2, 1, 1);


        verticalLayout_2->addLayout(gridLayout);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        italicOption = new QCheckBox(textFormatEdit);
        italicOption->setObjectName(QString::fromUtf8("italicOption"));

        verticalLayout_7->addWidget(italicOption);

        boldOption = new QCheckBox(textFormatEdit);
        boldOption->setObjectName(QString::fromUtf8("boldOption"));

        verticalLayout_7->addWidget(boldOption);


        verticalLayout_2->addLayout(verticalLayout_7);

        verticalSpacer_3 = new QSpacerItem(20, 294, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_3);


        horizontalLayout_5->addWidget(textFormatEdit);


        verticalLayout_8->addWidget(groupBox_2);

        tabs->addTab(colorsTab, QString());
        splitter->addWidget(tabs);

        verticalLayout->addWidget(splitter);


        retranslateUi(EditorConfigPage);

        tabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(EditorConfigPage);
    } // setupUi

    void retranslateUi(QWidget *EditorConfigPage)
    {
        EditorConfigPage->setWindowTitle(QApplication::translate("EditorConfigPage", "Form", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("EditorConfigPage", "Indentation", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("EditorConfigPage", "Indentation width:", 0, QApplication::UnicodeUTF8));
        indentWidth->setSpecialValueText(QString());
        indentWidth->setSuffix(QApplication::translate("EditorConfigPage", " spaces", 0, QApplication::UnicodeUTF8));
        indentWidth->setPrefix(QString());
        spaceIndent->setText(QApplication::translate("EditorConfigPage", "Use spaces instead of tabs", 0, QApplication::UnicodeUTF8));
        groupBox_4->setTitle(QApplication::translate("EditorConfigPage", "Code Evaluation", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("EditorConfigPage", "Blink duration:", 0, QApplication::UnicodeUTF8));
        blinkDuration->setSuffix(QApplication::translate("EditorConfigPage", "ms", 0, QApplication::UnicodeUTF8));
        stepForwardEvaluation->setText(QApplication::translate("EditorConfigPage", "Step forward after evaluating a line", 0, QApplication::UnicodeUTF8));
        tabs->setTabText(tabs->indexOf(tab), QApplication::translate("EditorConfigPage", "Behavior", 0, QApplication::UnicodeUTF8));
        editorLineWrap->setText(QApplication::translate("EditorConfigPage", "Wrap text in code editor", 0, QApplication::UnicodeUTF8));
        postWindowLineWrap->setText(QApplication::translate("EditorConfigPage", "Wrap text in post window", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("EditorConfigPage", "Post window scrollback:", 0, QApplication::UnicodeUTF8));
        postWindowScrollback->setSuffix(QApplication::translate("EditorConfigPage", " lines", 0, QApplication::UnicodeUTF8));
        tabs->setTabText(tabs->indexOf(tab_2), QApplication::translate("EditorConfigPage", "Display", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("EditorConfigPage", "Font", 0, QApplication::UnicodeUTF8));
        onlyMonoFonts->setText(QApplication::translate("EditorConfigPage", "Show only monospaced", 0, QApplication::UnicodeUTF8));
        label_16->setText(QApplication::translate("EditorConfigPage", "Family:", 0, QApplication::UnicodeUTF8));
        label_15->setText(QApplication::translate("EditorConfigPage", "Size:", 0, QApplication::UnicodeUTF8));
        fontAntialias->setText(QApplication::translate("EditorConfigPage", "Antialias", 0, QApplication::UnicodeUTF8));
        groupBox_2->setTitle(QApplication::translate("EditorConfigPage", "Colors", 0, QApplication::UnicodeUTF8));
        label_13->setText(QApplication::translate("EditorConfigPage", "Background:", 0, QApplication::UnicodeUTF8));
        bgClearBtn->setText(QApplication::translate("EditorConfigPage", "X", 0, QApplication::UnicodeUTF8));
        label_14->setText(QApplication::translate("EditorConfigPage", "Foreground:", 0, QApplication::UnicodeUTF8));
        fgClearBtn->setText(QApplication::translate("EditorConfigPage", "X", 0, QApplication::UnicodeUTF8));
        italicOption->setText(QApplication::translate("EditorConfigPage", "Italic", 0, QApplication::UnicodeUTF8));
        boldOption->setText(QApplication::translate("EditorConfigPage", "Bold", 0, QApplication::UnicodeUTF8));
        tabs->setTabText(tabs->indexOf(colorsTab), QApplication::translate("EditorConfigPage", "Font && Colors", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class EditorConfigPage: public Ui_EditorConfigPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGS_EDITOR_H
