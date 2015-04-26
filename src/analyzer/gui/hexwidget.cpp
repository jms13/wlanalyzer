#include <QActionGroup>
#include <QMouseEvent>
#include <QPalette>
#include <QPainter>
#include <QVariant>
#include <QFontDatabase>
#include <QScrollBar>
#ifdef DEBUG_BUILD
#   include<QFileDialog>
#   include<QMessageBox>
#endif
#include "gui/hexwidget.h"
#include "gui/hexwidget.moc"


const char *HEX_PATTERN = "0123456789abcdef";

template <typename T>
QString numToHexStr(T val)
{
    // buffer is the size of number of nibbles in number + trailing '\0'
    const unsigned NUM_NIBBLES = sizeof(T) * 2;
    char buf[NUM_NIBBLES + 1] = {false,};
    for(int i = NUM_NIBBLES -1; i >= 0; --i)
    {
        buf[i] = HEX_PATTERN[val & 0xf];
        val >>= 4;
    }
    return QString(buf);
}

template <typename T>
QString numToBinStr(T val)
{
    // buffer is the size of number of bits in number + trailing '\0'
    const unsigned NUM_BITS = sizeof(T) * 8;
    char buf[NUM_BITS + 1] = {false,};
    for(int i = NUM_BITS -1; i >= 0; --i)
    {
        buf[i] = '0' + (val & 0x1);
        val >>= 1;
    }
    return QString(buf);
}

// only base ASCII printable chars are interesting
static char ascii(char c)
{
    if(c < ' ' || c > '~')
        return '.';
    return c;
}

HexWidget::HexWidget(QWidget *parent) :
    QAbstractScrollArea(parent),
    m_format(DisplayFormat::HEX),
    m_numBytesInLine(16),
    m_numAddressChars(4),
    m_highlight_addr(0),
    m_highlight_len(0)
{

    // setup context menu with required options
    auto group = new QActionGroup(this);
    auto action = group->addAction(tr("Display data as Bit Field"));
    action->setCheckable(true);
    action->setData(qVariantFromValue(DisplayFormat::BIT_FIELD));
    action = group->addAction(tr("Display data as HEX"));
    action->setCheckable(true);
    action->setChecked(true);
    action->setData(qVariantFromValue(DisplayFormat::HEX));
    m_contextMenu.addActions(group->actions());
    connect(group, SIGNAL(triggered(QAction*)), this, SLOT(setRawDataDisplayFormat(QAction*)));

    // setup zoom controls
    m_contextMenu.addSeparator();
    action = new QAction(QIcon::fromTheme("zoom-in"), tr("Zoom in"), this);
    m_contextMenu.addAction(action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(zoomIn()));
    action = new QAction(QIcon::fromTheme("zoom-out"), tr("Zoom out"), this);
    m_contextMenu.addAction(action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(zoomOut()));

#ifdef DEBUG_BUILD
    // setup direct file opening context menu option
    m_contextMenu.addSeparator();
    action = new QAction(QIcon::fromTheme("document-open"), tr("[Debug] Open File"), this);
    m_contextMenu.addAction(action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(debugFileOpen()));
#endif

    ensureMonospacedFont();
    recalculateFontMetrics();
}

void HexWidget::setData(QIODevice* data)
{
    Q_ASSERT(data);
    //only random access QIODevices have semantics reasonable for this widget
    Q_ASSERT(data->isSequential() == false);
    Q_ASSERT(data->isReadable());
    m_data.reset(data);
    // previous highlight has no meaning now so reset it
    m_highlight_addr = m_highlight_len = 0;
    recalculateFontMetrics();
}

void HexWidget::setData(const QByteArray& data)
{
    auto buf = new QBuffer();
    buf->setData(data);
    buf->open(QIODevice::ReadOnly);
    setData(buf);
}

void HexWidget::recalculateFontMetrics()
{
    const QFontMetrics &fm = fontMetrics();
    m_lineHeight = fm.lineSpacing();
    m_textYOffset =  m_lineHeight - fm.descent();
    qint64 dsize = dataSize();

    if(m_format == DisplayFormat::HEX)
        m_numBytesInLine = 16;
    else
        m_numBytesInLine = 6;

    m_numLines = dsize / m_numBytesInLine + 1;
    m_numVisibleLines = qMin(static_cast<qint64>(viewport()->height() / m_lineHeight), m_numLines);

    // calculate number of characters requred to represent the address
    m_numAddressChars = 4;
    while((dsize >>= 16) > 0)
        m_numAddressChars += 4; // plus 1 char inter word spacing

    m_characterWidth = fm.boundingRect(QChar('W')).width();
    m_textMargin = m_characterWidth;
    const int colMargin = 2 * m_textMargin;

    const int addrColumnWidth = colMargin + m_characterWidth * m_numAddressChars;
    // TODO in case we would like to implement variable byte spacing/grouping
    // this needs rewriting
    const int rawColumnWidth = m_textMargin + rawByteDisplayWidth() * m_numBytesInLine;
    const int printableColumnWidth = colMargin + printableByteDisplayWidth() * m_numBytesInLine;
    const int lineWidth = addrColumnWidth + rawColumnWidth + printableColumnWidth;

    // manage correct scroll and scrollbar ranges
    horizontalScrollBar()->setRange(0, lineWidth + m_textMargin - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());
    verticalScrollBar()->setRange(0, m_numLines - m_numVisibleLines);
    verticalScrollBar()->setPageStep(m_numVisibleLines);

    m_vp.setRect(horizontalScrollBar()->value(), verticalScrollBar()->value(),
                 viewport()->width(), viewport()->height());

    m_addrColumn.setRect( -m_vp.x(), 0, addrColumnWidth, m_vp.height());
    m_rawColumn.setRect( m_addrColumn.right(),0, rawColumnWidth, m_vp.height());
    m_printableColumn.setRect(m_rawColumn.right(),0, printableColumnWidth, m_vp.height());
}

qint64 HexWidget::dataSize() const
{
    //XXX - QAbstractScrollView is not able to handle larger viewport sizes so
    // for now the datasize 0xffffffff seams to be the limit of what we can display.
    if(m_data)
        return m_data->size();

    return 0;
}

QByteArray HexWidget::dataLineAtAddr(qint64 addr) const
{
    Q_ASSERT(dataSize() >= addr);
    Q_ASSERT(addr >= 0);
    Q_ASSERT(addr % m_numBytesInLine == 0);
    if(!m_data)
        return QByteArray();

    if(m_data->pos() != addr)
        m_data->seek(addr);

    return m_data->read(m_numBytesInLine);
}

HexWidget::RegionId HexWidget::regionAtPos(const QPoint& pos) const
{
    if(pos.x() < m_addrColumn.left() || pos.x() > m_printableColumn.right())
        return RegionId::INVALID;
    if(pos.x() > m_printableColumn.left())
        return RegionId::PRINTABLE;
    if(pos.x() > m_rawColumn.left())
        return RegionId::RAW;
    else
        return RegionId::ADDRESS;
}

qint64 HexWidget::addrAtPos(const QPoint& pos, bool& ok) const
{
    RegionId reg = regionAtPos(pos);
    ok = true;

    int line_num = pos.y() / m_lineHeight + m_vp.y();
    int column_num = 0;

    if( reg == RegionId::RAW)
        column_num =  (pos.x() - m_rawColumn.left() - m_textMargin) / rawByteDisplayWidth();
    else if(reg == RegionId::PRINTABLE)
        column_num =  (pos.x() - m_printableColumn.left() - m_textMargin) / printableByteDisplayWidth();

    qint64 addr = addrAtLineAndColumn(line_num, column_num);
    if (reg == RegionId::INVALID || column_num >= m_numBytesInLine
            || !addrValid(addr))
    {
        ok = false;
        addr = 0;
    }
    return addr;
}

qint64 HexWidget::addrAtLineAndColumn(qint64 line, int column) const
{
    return line * m_numBytesInLine + column;
}

QPoint HexWidget::posAtAddr(qint64 addr, RegionId reg, bool& ok) const
{
    //TODO this will be needed when we would like to scroll to line/column on
    //outside selection
    Q_UNUSED(addr);
    Q_UNUSED(reg);
    Q_UNUSED(ok);
    return QPoint();
}

bool HexWidget::addrValid(qint64 addr) const
{
    return m_data && addr >= 0 && addr < dataSize();
}

// TODO rewrite this with bye index variable in case we would like
// to implement variable byte spacing/grouping
int HexWidget::rawByteDisplayWidth() const
{
    if(m_format == DisplayFormat::HEX)
    {
        return m_characterWidth * 3;
    }else
    {
        return m_characterWidth * 9;
    }
}

int HexWidget::printableByteDisplayWidth() const
{
    return m_characterWidth;
}

void HexWidget::highlight(qint64 start_addr, qint64 len)
{
    if(!addrValid(start_addr))
        return;

    Q_ASSERT(start_addr >=0 && start_addr <= dataSize());
    Q_ASSERT(len >=0);
    m_highlight_addr = start_addr;
    m_highlight_len = len;
    viewport()->update();
}

void HexWidget::setRawDataDisplayFormat(QAction *action)
{
    bool data_ok = true;
    DisplayFormat format = static_cast<DisplayFormat>(action->data().toInt(&data_ok));

    Q_ASSERT(data_ok);
    if(format != m_format)
    {
        m_format = format;
        recalculateFontMetrics();
        viewport()->update();
    }
}

void HexWidget::zoomIn()
{
    resizeFont(1);
}

void HexWidget::zoomOut()
{
    resizeFont(-1);
}

#ifdef DEBUG_BUILD
void HexWidget::debugFileOpen()
{
    QFileDialog dialog(this, tr("[Debug] Open File"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setOption(QFileDialog::ReadOnly);
    if(dialog.exec())
    {
        QString name = dialog.selectedFiles()[0];
        auto debug_file = new QFile(name);
        debug_file->open(QIODevice::ReadOnly);
        if(debug_file->isReadable())
        {
            setData(debug_file);
        }
        else
        {
            delete debug_file;
            QMessageBox::warning(this, tr("Cannot read file"), name);
        }
    }
}
#endif

void HexWidget::contextMenuEvent(QContextMenuEvent *event)
{
    m_contextMenu.exec(event->globalPos());
}

void HexWidget::scrollContentsBy(int dx, int dy)
{
    // scroll vertically only by full line increments
    viewport()->scroll(dx, dy * m_lineHeight);
    recalculateFontMetrics(); // TODO - use lighter version of this call
    viewport()->update();
}

void HexWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(viewport());
    painter.fillRect(QRect(QPoint(0, 0), m_vp.size()), palette().background());
    painter.fillRect(m_addrColumn, palette().dark());
    painter.fillRect(m_printableColumn, palette().dark());

    int raw_entry_w = rawByteDisplayWidth();
    int print_entry_w = printableByteDisplayWidth();

    // paint consecutive lines
    for(int j = 0; j <= m_numVisibleLines; ++j)
    {
        int line_y = m_lineHeight * j;
        int text_line_y = line_y + m_textYOffset;
        qint64 address = addrAtLineAndColumn(m_vp.y() + j, 0);

        if(!addrValid(address))
            break;

        painter.drawText(QPointF(m_addrColumn.left() + m_textMargin, text_line_y),
                         numToHexStr(address).right(m_numAddressChars));

        QByteArray arr = dataLineAtAddr(address);
        // paint the actual values
        for(int i = 0; i < arr.size(); ++i)
        {
            QString text;
            char c = arr.at(i);
            if(m_format == DisplayFormat::HEX)
                text = numToHexStr(c);
            else
                text = numToBinStr(c);

            painter.save();
            if(m_highlight_len > 0 && (address + i) >= m_highlight_addr && (address + i) < (m_highlight_addr + m_highlight_len))
            {
                // XXX refactor thiese rects and offsets once implementing variable byte spacing / grouping
                painter.fillRect(QRectF(QPointF(m_rawColumn.left() + raw_entry_w * i, line_y), QSizeF(raw_entry_w + m_textMargin, m_lineHeight)), palette().highlight());
                painter.fillRect(QRectF(QPointF(m_printableColumn.left() + m_textMargin + print_entry_w * i, line_y), QSizeF(print_entry_w, m_lineHeight)), palette().highlight());
                painter.setPen(palette().highlightedText().color());
            }

            painter.drawText(QPointF(m_rawColumn.left() + m_textMargin + raw_entry_w * i, text_line_y), text);
            painter.drawText(QPointF(m_printableColumn.left() + m_textMargin + print_entry_w * i, text_line_y),
                             QString(ascii(c)));
            painter.restore();
        }
    }

    // draw vertical lines separating the widget columns
    painter.drawLine(m_addrColumn.left(), m_addrColumn.top(), m_addrColumn.left(), m_rawColumn.bottom());
    painter.drawLine(m_rawColumn.left(), m_rawColumn.top(), m_rawColumn.left(), m_rawColumn.bottom());
    painter.drawLine(m_printableColumn.left(), m_printableColumn.top(), m_printableColumn.left(), m_printableColumn.bottom());
    painter.drawLine(m_printableColumn.right(), m_printableColumn.top(), m_printableColumn.right(), m_printableColumn.bottom());
}

void HexWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        resizeFont(event->delta() > 0 ? 1 : -1 );
        return; // avoid scrolling when we zoom
    }
    QAbstractScrollArea::wheelEvent(event);
}

void HexWidget::resizeEvent(QResizeEvent *event)
{
    recalculateFontMetrics();
    QAbstractScrollArea::resizeEvent(event);
}

void HexWidget::resizeFont(int sizeIncrement)
{
    QFont font = this->font();
    const int newSize = font.pointSize() + sizeIncrement;
    if (newSize <= 0)
        return;
    font.setPointSize(newSize);
    setFont(font);
    recalculateFontMetrics();
}

void HexWidget::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton)
        return;

    bool ok = false;
    qint64 addr = addrAtPos(e->pos(), ok);
    if(ok && addrValid(addr))
        emit addressSelected(addr);
}

void HexWidget::mouseMoveEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
}

void HexWidget::ensureMonospacedFont()
{
    // setup monospaced font and fontmetrics
#if QT_VERSION >= 0x050200
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#else
    QFont font("monospace");
#endif
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    setFont(font);
}

void HexWidget::drawDebug(QPainter& painter) const
{
    bool ok;
    painter.drawText(m_cursorPos, QString("0x%1 : %2").arg(numToHexStr((quint16)addrAtPos(m_cursorPos, ok))).arg((quint16)addrAtPos(m_cursorPos, ok)));
    const QColor color_g(116,214,0, 50);
    const QColor color_r(239,65,53, 100);
    RegionId reg = regionAtPos(m_cursorPos);
    switch(reg)
    {
        case RegionId::INVALID:
            painter.fillRect(QRect(QPoint(0,0), m_vp.size()), color_r);
            break;
        case RegionId::ADDRESS:
            painter.fillRect(m_addrColumn,  color_g);
            break;
        case RegionId::RAW:
            painter.fillRect(m_rawColumn, color_g);
            break;
        case RegionId::PRINTABLE:
            painter.fillRect(m_printableColumn, color_g);
            break;
        default:
            Q_ASSERT(false);
    }
}