/********************************************************************************
** Form generated from reading UI file 'color_palette_widget.ui'
**
** Created by: Qt User Interface Compiler version 5.11.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_COLOR_PALETTE_WIDGET_H
#define UI_COLOR_PALETTE_WIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "QtColorWidgets/swatch.hpp"

namespace color_widgets {

class Ui_ColorPaletteWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    QComboBox *palette_list;
    QWidget *group_edit_list;
    QHBoxLayout *horizontalLayout_2;
    QToolButton *button_palette_open;
    QToolButton *button_palette_new;
    QToolButton *button_palette_duplicate;
    color_widgets::Swatch *swatch;
    QWidget *group_edit_palette;
    QHBoxLayout *horizontalLayout;
    QToolButton *button_palette_delete;
    QToolButton *button_palette_revert;
    QToolButton *button_palette_save;
    QSpacerItem *horizontalSpacer;
    QToolButton *button_color_add;
    QToolButton *button_color_remove;

    void setupUi(QWidget *color_widgets__ColorPaletteWidget)
    {
        if (color_widgets__ColorPaletteWidget->objectName().isEmpty())
            color_widgets__ColorPaletteWidget->setObjectName(QStringLiteral("color_widgets__ColorPaletteWidget"));
        color_widgets__ColorPaletteWidget->resize(227, 186);
        verticalLayout = new QVBoxLayout(color_widgets__ColorPaletteWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        palette_list = new QComboBox(color_widgets__ColorPaletteWidget);
        palette_list->setObjectName(QStringLiteral("palette_list"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(palette_list->sizePolicy().hasHeightForWidth());
        palette_list->setSizePolicy(sizePolicy);

        horizontalLayout_3->addWidget(palette_list);

        group_edit_list = new QWidget(color_widgets__ColorPaletteWidget);
        group_edit_list->setObjectName(QStringLiteral("group_edit_list"));
        horizontalLayout_2 = new QHBoxLayout(group_edit_list);
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        button_palette_open = new QToolButton(group_edit_list);
        button_palette_open->setObjectName(QStringLiteral("button_palette_open"));
        QIcon icon;
        QString iconThemeName = QStringLiteral("document-open");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon = QIcon::fromTheme(iconThemeName);
        } else {
            icon.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        button_palette_open->setIcon(icon);

        horizontalLayout_2->addWidget(button_palette_open);

        button_palette_new = new QToolButton(group_edit_list);
        button_palette_new->setObjectName(QStringLiteral("button_palette_new"));
        QIcon icon1;
        iconThemeName = QStringLiteral("document-new");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon1 = QIcon::fromTheme(iconThemeName);
        } else {
            icon1.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        button_palette_new->setIcon(icon1);

        horizontalLayout_2->addWidget(button_palette_new);

        button_palette_duplicate = new QToolButton(group_edit_list);
        button_palette_duplicate->setObjectName(QStringLiteral("button_palette_duplicate"));
        QIcon icon2;
        iconThemeName = QStringLiteral("edit-copy");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon2 = QIcon::fromTheme(iconThemeName);
        } else {
            icon2.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        button_palette_duplicate->setIcon(icon2);

        horizontalLayout_2->addWidget(button_palette_duplicate);


        horizontalLayout_3->addWidget(group_edit_list);


        verticalLayout->addLayout(horizontalLayout_3);

        swatch = new color_widgets::Swatch(color_widgets__ColorPaletteWidget);
        swatch->setObjectName(QStringLiteral("swatch"));

        verticalLayout->addWidget(swatch);

        group_edit_palette = new QWidget(color_widgets__ColorPaletteWidget);
        group_edit_palette->setObjectName(QStringLiteral("group_edit_palette"));
        horizontalLayout = new QHBoxLayout(group_edit_palette);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        button_palette_delete = new QToolButton(group_edit_palette);
        button_palette_delete->setObjectName(QStringLiteral("button_palette_delete"));
        QIcon icon3;
        iconThemeName = QStringLiteral("document-close");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon3 = QIcon::fromTheme(iconThemeName);
        } else {
            icon3.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        button_palette_delete->setIcon(icon3);

        horizontalLayout->addWidget(button_palette_delete);

        button_palette_revert = new QToolButton(group_edit_palette);
        button_palette_revert->setObjectName(QStringLiteral("button_palette_revert"));
        QIcon icon4;
        iconThemeName = QStringLiteral("document-revert");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon4 = QIcon::fromTheme(iconThemeName);
        } else {
            icon4.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        button_palette_revert->setIcon(icon4);

        horizontalLayout->addWidget(button_palette_revert);

        button_palette_save = new QToolButton(group_edit_palette);
        button_palette_save->setObjectName(QStringLiteral("button_palette_save"));
        QIcon icon5;
        iconThemeName = QStringLiteral("document-save");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon5 = QIcon::fromTheme(iconThemeName);
        } else {
            icon5.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        button_palette_save->setIcon(icon5);

        horizontalLayout->addWidget(button_palette_save);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        button_color_add = new QToolButton(group_edit_palette);
        button_color_add->setObjectName(QStringLiteral("button_color_add"));
        QIcon icon6;
        iconThemeName = QStringLiteral("list-add");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon6 = QIcon::fromTheme(iconThemeName);
        } else {
            icon6.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        button_color_add->setIcon(icon6);

        horizontalLayout->addWidget(button_color_add);

        button_color_remove = new QToolButton(group_edit_palette);
        button_color_remove->setObjectName(QStringLiteral("button_color_remove"));
        QIcon icon7;
        iconThemeName = QStringLiteral("list-remove");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon7 = QIcon::fromTheme(iconThemeName);
        } else {
            icon7.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        button_color_remove->setIcon(icon7);

        horizontalLayout->addWidget(button_color_remove);


        verticalLayout->addWidget(group_edit_palette);


        retranslateUi(color_widgets__ColorPaletteWidget);

        QMetaObject::connectSlotsByName(color_widgets__ColorPaletteWidget);
    } // setupUi

    void retranslateUi(QWidget *color_widgets__ColorPaletteWidget)
    {
#ifndef QT_NO_TOOLTIP
        button_palette_open->setToolTip(QApplication::translate("color_widgets::ColorPaletteWidget", "Open a new palette from file", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        button_palette_new->setToolTip(QApplication::translate("color_widgets::ColorPaletteWidget", "Create a new palette", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        button_palette_duplicate->setToolTip(QApplication::translate("color_widgets::ColorPaletteWidget", "Duplicate the current palette", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        button_palette_delete->setToolTip(QApplication::translate("color_widgets::ColorPaletteWidget", "Delete the current palette", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        button_palette_revert->setToolTip(QApplication::translate("color_widgets::ColorPaletteWidget", "Revert changes to the current palette", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        button_palette_save->setToolTip(QApplication::translate("color_widgets::ColorPaletteWidget", "Save changes to the current palette", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        button_color_add->setToolTip(QApplication::translate("color_widgets::ColorPaletteWidget", "Add a color to the palette", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        button_color_remove->setToolTip(QApplication::translate("color_widgets::ColorPaletteWidget", "Remove the selected color from the palette", nullptr));
#endif // QT_NO_TOOLTIP
        Q_UNUSED(color_widgets__ColorPaletteWidget);
    } // retranslateUi

};

} // namespace color_widgets

namespace color_widgets {
namespace Ui {
    class ColorPaletteWidget: public Ui_ColorPaletteWidget {};
} // namespace Ui
} // namespace color_widgets

#endif // UI_COLOR_PALETTE_WIDGET_H
