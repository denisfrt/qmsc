#include "qmsc_settings.h"
#include <QGuiApplication>
#include <QSettings>
#include <QStyleHints>
#include "qmsc_main_window.h"

namespace qmsc {

MscSettings::MscSettings(MscMainWindow *parent)
    : QSettings(QSettings::IniFormat, QSettings::UserScope, "qmsc")
    , _parent(parent)
    , _maxRecentFiles(10)
{
}

MscSettings::MscSettings::Config::Config()
    : _mscFontSize(ast::defs::fontSize)
    , _editFontSize(ast::defs::fontSize)
    , _mscFontName(getQString(ast::defs::fontName))

{
    _mscFont = QFont(_mscFontName, _mscFontSize);
    _editFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    _editFontName = _editFont.family();
    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        _colorSelection = QColor(0x3e3e3e);
    }
}

MscSettings::Config &MscSettings::config()
{
    return _config;
}

const QStringList &MscSettings::recentFiles() const
{
    return _recentFiles;
}

int MscSettings::Config::updateConfig(const Config &config)
{
    int modified = 0;
    if (_colorSelection != config._colorSelection) {
        modified |= eColorSelection;
        _colorSelection = config._colorSelection;
    }
    if (_colorBlock != config._colorBlock) {
        modified |= eColorBlock;
        _colorBlock = config._colorBlock;
    }
    if (_colorElement != config._colorElement) {
        modified |= eColorElement;
        _colorElement = config._colorElement;
    }
    if (_colorProperty != config._colorProperty) {
        modified |= eColorProperty;
        _colorProperty = config._colorProperty;
    }
    if (_colorString != config._colorString) {
        modified |= eColorString;
        _colorString = config._colorString;
    }
    if (_colorComment != config._colorComment) {
        modified |= eColorComment;
        _colorComment = config._colorComment;
    }
    if (_debug != config._debug) {
        modified |= eDebug;
        _debug = config._debug;
    }
    if (_lockInstances != config._lockInstances) {
        modified |= eLockInstances;
        _lockInstances = config._lockInstances;
    }
    if (_centerMsc != config._centerMsc) {
        modified |= eCenterMsc;
        _centerMsc = config._centerMsc;
    }
    if (_mscFontSize != config._mscFontSize || _mscFontName != config._mscFontName) {
        modified |= eMscFont;
        _mscFontSize = config._mscFontSize;
        _mscFontName = config._mscFontName;
        _mscFont = QFont(_mscFontName, _mscFontSize);
    }
    if (_editFontSize != config._editFontSize || _editFontName != config._editFontName) {
        modified |= eEditFont;
        _editFontSize = config._editFontSize;
        _editFontName = config._editFontName;
        _editFont = QFont(_editFontName, _editFontSize);
    }
    return modified;
}

void MscSettings::save()
{
    setValue("window/geometry", _parent->saveGeometry());
    setValue("window/state", _parent->saveState());
    setValue("qmsc/recentfiles", _recentFiles);
    setValue("qmsc/maxrecentfiles", _maxRecentFiles);

    setValue("qmsc/debug", _config._debug);
    setValue("qmsc/lockinstances", _config._lockInstances);
    setValue("qmsc/centermsc", _config._centerMsc);
    setValue("qmsc/color/selection", _config._colorSelection.rgb());
    setValue("qmsc/color/block", _config._colorBlock.rgb());
    setValue("qmsc/color/element", _config._colorElement.rgb());
    setValue("qmsc/color/property", _config._colorProperty.rgb());
    setValue("qmsc/color/string", _config._colorString.rgb());
    setValue("qmsc/color/comment", _config._colorComment.rgb());
    setValue("qmsc/view/fontname", _config._mscFontName);
    setValue("qmsc/view/fontsize", _config._mscFontSize);
    setValue("qmsc/edit/fontname", _config._editFontName);
    setValue("qmsc/edit/fontsize", _config._editFontSize);

    sync();
}

void MscSettings::load()
{
    _parent->restoreGeometry(value("window/geometry").toByteArray());
    _parent->restoreState(value("window/state").toByteArray());
    _recentFiles = value("qmsc/recentfiles").toStringList();
    _maxRecentFiles = value("qmsc/maxrecentfiles", _maxRecentFiles).toInt();

    // clang-format off
    _config._debug = value("qmsc/debug", false).toBool();
    _config._lockInstances = value("qmsc/lockinstances", false).toBool();
    _config._centerMsc = value("qmsc/centermsc", false).toBool();
    _config._colorSelection = QColor(value("qmsc/color/selection", _config._colorSelection.rgba()).toUInt());
    _config._colorBlock = QColor(value("qmsc/color/block", _config._colorBlock.rgb()).toUInt());
    _config._colorElement = QColor(value("qmsc/color/element", _config._colorElement.rgb()).toUInt());
    _config._colorProperty = QColor(value("qmsc/color/property", _config._colorProperty.rgb()).toUInt());
    _config._colorString = QColor(value("qmsc/color/string", _config._colorString.rgb()).toUInt());
    _config._colorComment = QColor(value("qmsc/color/comment", _config._colorComment.rgb()).toUInt());
    _config._mscFontName = value("qmsc/view/fontname", _config._mscFontName).toString();
    _config._mscFontSize = value("qmsc/view/fontsize", _config._mscFontSize).toInt();
    _config._mscFont = QFont(_config._mscFontName, _config._mscFontSize);
    _config._editFontName = value("qmsc/edit/fontname", _config._editFontName).toString();
    _config._editFontSize = value("qmsc/edit/fontsize", _config._editFontSize).toInt();
    _config._editFont = QFont(_config._editFontName, _config._editFontSize);
    // clang-format on
}

void MscSettings::addToRecent(const QString &filePath)
{
    _recentFiles.removeAll(filePath);
    _recentFiles.prepend(filePath);
    while (_recentFiles.size() > _maxRecentFiles)
        _recentFiles.removeLast();
}

} // namespace qmsc