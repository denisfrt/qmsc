#pragma once
#include <QColor>
#include <QFont>
#include <QObject>
#include <QSettings>
#include <QSize>
#include <QString>
#include "ast.h"
#include "qmsc_utils.h"

namespace qmsc {

class MscMainWindow;

class MscSettings : public QSettings
{
    Q_OBJECT
public:
    enum eOptionFlag : int {
        eColorSelection = 0x1,
        eColorBlock = 0x2,
        eColorElement = 0x4,
        eColorProperty = 0x8,
        eColorString = 0x10,
        eColorComment = 0x20,
        eColorOptions = eColorSelection | eColorBlock | eColorElement | eColorProperty
                        | eColorString | eColorComment,
        eDebug = 0x40,
        eLockInstances = 0x80,
        eCenterMsc = 0x100,
        eMscFont = 0x200,
        eEditFont = 0x400
    };
    struct Config
    {
        // selection color
        QColor _colorSelection{0xbababa};
        // text syntax color: block
        QColor _colorBlock{0xd65645};
        // text syntax color: element
        QColor _colorElement{0x45c6d6};
        // text syntax color: property
        QColor _colorProperty{0xd6c540};
        // text syntax color: string
        QColor _colorString{0xd69545};
        // text syntax color: comment
        QColor _colorComment{0xa8abb0};

        // debug enabled
        bool _debug = false;
        // locked instances enabled
        bool _lockInstances = false;
        // centered msc enable
        bool _centerMsc = false;

        // global msc offset from left and offset from top
        QSize _offset{20, 20};
        // x space between instances
        int _instanceXSpace = ast::defs::instanceXSpace;
        // instance rectangle default size (before content correction)
        QSize _instanceSize{60, 24};

        // margin between elements
        QSize _elementMargin{6, ast::defs::itemYSpace};
        // message arrow height
        int _yArrowHeight = 8;
        // margin between element and its anno
        int _yAnnoMargin = 15;
        // margin between elements
        QSize _refReduxMargin{16, 16};

        // margin added for each lines of text
        QSize _textLineMargin{2, 0};

        // Font
        QFont _mscFont;
        QFont _editFont;
        int _mscFontSize;
        int _editFontSize;
        QString _mscFontName;
        QString _editFontName;

        Config();
        int updateConfig(const Config &config);
    };

public:
    explicit MscSettings(MscMainWindow *parent);
    virtual ~MscSettings() = default;

public:
    Config &config();
    const QStringList &recentFiles() const;
    void save();
    void load();
    void addToRecent(const QString &filePath);

private:
    MscMainWindow *_parent;
    QStringList _recentFiles;
    int _maxRecentFiles;
    Config _config;
};

} // namespace qmsc