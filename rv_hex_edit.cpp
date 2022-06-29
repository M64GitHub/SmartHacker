#include "rv_hex_edit.h"
#include <rv_hex_edit_undocommands.h>


rv_hex_edit::rv_hex_edit(QWidget *parent) :
    QAbstractScrollArea(parent)
{
    editor = 0;
    model = 0;
    original_dataptr = 0;

    current_dataptr = 0;
    current_offset = 0;
    current_data_len = 0;

    single_range = -1;

    set_cursor_data(&cursor, 0);
    set_cursor_data(&shadow_cursor, 0);

    have_focus = 0;
    show_col_ascii = true;
    show_infoline = true;
    show_grid = true;
    have_color_ranges = false;
    have_data = false;

    selection_start = -1;
    selection_end   = -1;
    selecting       = false;
    have_selection  = false;

    init_std_geometry();

    color_hex_vals  = QColor(0x30, 0x30, 0x30);
    color_bg1_vals  = QColor(0xf0, 0xe8, 0xf8);
    color_bg2_vals  = QColor(0xe0, 0xd8, 0xe8);
    color_asc_vals  = QColor(0x60, 0x60, 0x60);
    color_asc_vals2  = QColor(0x80, 0x80, 0x80);

    color_cursor    = QColor(0x40, 0x40, 0x60);
    color_cursor2   = QColor(0x80, 0x80, 0xa0);
    color_grid      = QColor(0xc0, 0xc0, 0xc0);
    color_grid2     = QColor(0xe0, 0xe0, 0xe0);

    color_grid_info = QColor(0xc0, 0xc0, 0xc0);
    color_bg_info   = QColor(0xff, 0xff, 0xff);
    color_txt_info  = QColor(0x30, 0x30, 0x30);

    color_bg_hdr            = QColor(0x70, 0x70, 0x70);
    color_bg_hdr_selected   = QColor(0x30, 0x30, 0x30);
    color_txt_hdr   = QColor(0xff, 0xff, 0xff);

    color_shadow_cursor = QColor(0xa8, 0xa8, 0xa8, 0x80);
    color_shadow_bg_hdr_selected   = QColor(0xa8, 0xa8, 0xa8);

    selection_bgcolor = QColor(0x80, 0xa0, 0xb0);
    selection_txtcolor = QColor(0xff, 0xff, 0xff);

    shift_down = false;

    selection.hv = this;

    QFont   f;
    f.setFamily("Liberation Mono");
    f.setPointSize(9);
    setFont(f);
    setMouseTracking(true);

    undo_stack = new QUndoStack(this);    
    setup_actions();
    setup_menus();
    set_can_paste(false);
}

void rv_hex_edit::init_std_geometry()
{
    // int w, int h, int bw, int bh, int tw, int th
    set_cellgeometry(4+34+8+4, 30+4-2,
                     4,4,
                     34+0, 30+0-2);


//    set_cellgeometry(27, 20,
//                     30, 16,
//                     25, 12);

    header_offs_width = 104;
    lmargin_col_ascii = 5;

    info_line_height = 34;
    header_col_height = 32;
    hdr_content_space = 3;
}

void rv_hex_edit::set_clipping_region(unsigned int offs, unsigned int len, bool b_use_va)
{    
    if(!have_data) return;

    if((offs + len) <= original_len) {
        current_offset = offs;
        current_data_len = len;
        current_dataptr = original_dataptr + offs;
    }

    recalc_scrollbars();
}

void rv_hex_edit::paintEvent(QPaintEvent *event)
{
    if(!have_data) return;

    QPainter painter(viewport());
    QPainter *p = &painter;

    int row = verticalScrollBar()->value();

    char buf[32];
    QString tmp;

    unsigned int index_row;

    if(event->region().boundingRect().intersects(
                QRect(
                (MARGIN_LEFT + header_offs_width),
                MARGIN_TOP,
                    viewport()->width() - header_offs_width - MARGIN_LEFT,
                    header_col_height
                )))
    paint_hdr_col(p, event->region().boundingRect());

    for(int i=0; i<num_lines; i++)    {
        index_row = i + row;
        // end of file ?
        if(index_row >= lines_count) break;


        // -- draw offset header --

        if(event->region().boundingRect().intersects(
                    QRect(
                    MARGIN_LEFT,
                    MARGIN_TOP + header_col_height +  i * line_height,
                        header_offs_width - lmargin_col_ascii,
                        line_height
                    ))) {
        // cursor in our row?
        if( (cursor.row) == index_row) {
            p->fillRect(MARGIN_LEFT,
                              MARGIN_TOP + header_col_height +  i * line_height,
                              header_offs_width - lmargin_col_ascii - 1,
                              line_height -1,
                              color_bg_hdr_selected
                              );
            p->setPen(Qt::white);
        } else if( (shadow_cursor.screen_row) == i) {
            // shadow cursor
            p->fillRect(MARGIN_LEFT,
                              MARGIN_TOP + header_col_height +  i * line_height,
                              header_offs_width - lmargin_col_ascii - 1,
                              line_height -1,
                              color_shadow_bg_hdr_selected
                              );
            p->setPen(Qt::white);
        } else {
            p->fillRect(MARGIN_LEFT,
                              MARGIN_TOP + header_col_height +  i * line_height,
                              header_offs_width - lmargin_col_ascii - 1,
                              line_height -1,
                              color_bg_hdr
                              );
            p->setPen(color_txt_hdr);
        }

        ::snprintf(buf, 16, "%08X", index_row * 16);

        tmp = QString::fromLatin1(buf);
        //tmp+= "row: " + QString::number(row) + "i: " + QString::number(i);
        p->drawText(MARGIN_LEFT + 4,
                         MARGIN_TOP + header_col_height + i * line_height
                          + line_height
                          - ((cell_geometry.height - cell_geometry.txt_height)/2)
                          - 10,
                         tmp);
        }

        // -- paint cell --
        for(int j=0; j<16; j++) {
            if(event->region().boundingRect().intersects(
                        QRect(
                        (j*cell_geometry.width + header_offs_width + MARGIN_LEFT),
                        (i*line_height + header_col_height + MARGIN_TOP),
                            cell_geometry.width, cell_geometry.height
                        )))
            paint_cell(p, j, i);
        }


        // -- paint ascii col --
        if(event->region().boundingRect().intersects(
                    QRect(
                    (16*cell_geometry.width + header_offs_width + MARGIN_LEFT),
                    (i*line_height + header_col_height + MARGIN_TOP),
                        16 * 9, cell_geometry.height
                    )))
        paint_asc_col(p, i);

    }

    paint_grid(p);

    if(event->region().boundingRect().intersects(
                QRect(
                0,
                viewport()->height() - line_height - 5,
                    viewport()->width(), 30
                ))) {
        if(have_color_ranges)paint_infoline(p);
        if(single_range >= 0) {
            paint_range_desc(p, single_range);
        }
    }
}

void rv_hex_edit::recalc_scrollbars()
{
    if(!have_data) return;
    lines_count = current_data_len / 16;
    if(current_data_len % 16) lines_count++;
    if(!color_ranges.isEmpty()) {
        num_lines = (viewport()->height()
                      - MARGIN_TOP - info_line_height - header_col_height)
                     / line_height;
    }
    else {
        num_lines = (viewport()->height()
                  - MARGIN_TOP - header_col_height)
                 / line_height;
    }
    verticalScrollBar()->setMaximum(lines_count - num_lines);
    verticalScrollBar()->setMinimum(0);
}

void rv_hex_edit::resizeEvent(QResizeEvent *)
{
    recalc_scrollbars();
}

void rv_hex_edit::keyPressEvent(QKeyEvent *event)
{
    if(!have_data) return;

    bool repaint_me = false;
    unsigned int offs = cursor.offset;

    switch(event->key()) {
    case Qt::Key_Right:
        if(offs < current_data_len) {
            offs++;

            if((offs / 16) >= (verticalScrollBar()->value() + num_lines))
                verticalScrollBar()->setValue(
                            verticalScrollBar()->value()+1);
            else repaint_me = true;
        }
        break;
    case Qt::Key_Left:
        if(offs) {
            offs--;

            if((offs / 16) < verticalScrollBar()->value())
                verticalScrollBar()->setValue(
                            verticalScrollBar()->value()-1);
            else repaint_me = true;
        }
        break;


    case Qt::Key_Down:
        if((offs + 16) < current_data_len) {
            offs+=16;

            if((offs / 16) >= (verticalScrollBar()->value() + num_lines))
                verticalScrollBar()->setValue(
                            verticalScrollBar()->value()+1);
            else repaint_me = true;
        }
        break;

    case Qt::Key_Up:
        if((offs >= 16)) {
            offs-=16;

            if((offs / 16) < verticalScrollBar()->value())
                verticalScrollBar()->setValue(
                            verticalScrollBar()->value()-1);
            else repaint_me = true;
        }
        break;
    }

    if(repaint_me) {
        old_cursor = cursor;
        set_cursor_data(&old_cursor, old_cursor.offset); // update to clear properly

        set_cursor_data(&cursor, offs);

        if(cursor.offset != old_cursor.offset) {
            repaint_cursor(&old_cursor, &cursor);
            emit cursor_offset_changed_to(cursor.offset + current_offset);
        }
    }
}

void rv_hex_edit::keyReleaseEvent(QKeyEvent *)
{

}

void rv_hex_edit::focusInEvent(QFocusEvent *)
{
    if(!have_data) return;
    have_focus = true;
    viewport()->repaint();
}

void rv_hex_edit::focusOutEvent(QFocusEvent *)
{
    if(!have_data) return;
    have_focus = false;
    viewport()->repaint();
}

int rv_hex_edit::get_total_height()
{
   recalc_scrollbars();
   return (current_data_len / 16 + 1) * line_height
           + MARGIN_TOP  + header_col_height + info_line_height / 2;
}

int rv_hex_edit::get_total_width()
{
    return MARGIN_LEFT + header_offs_width + 16 * cell_geometry.width
            + lmargin_col_ascii + 2 + 16*9 + 2;
}

void rv_hex_edit::mousePressEvent(QMouseEvent *event)
{
    if(!have_data) return;
    if(event->button() == Qt::LeftButton) {
        const int x = event->x();
        const int y = event->y();

        handle_click(x, y);
        if(editor) { editor->close(); editor=0; }

        // new selection method via shift+click
        if(event->modifiers() & Qt::ShiftModifier) {
            selection_start = old_cursor.offset;
            selection_end = cursor.offset;
            handle_selection();
            return;
        }

        selection_start = cursor.offset;
        selection_end = selection_start;
        selecting = true;
        if(have_selection) {
            have_selection = false;
            viewport()->repaint();
            emit selection_cleared();
        }
    }
}

void rv_hex_edit::mouseReleaseEvent(QMouseEvent *event)
{
    if(!have_data) return;
    if(event->button() == Qt::LeftButton) {
        const int x = event->x();
        const int y = event->y();

        set_cursor_data(&tmp_cursor, x, y);
        if(selecting) {
            selection_end = tmp_cursor.offset;
            handle_selection();
        }
    }
}

void rv_hex_edit::handle_selection()
{
    if(selection_start > selection_end) {
        unsigned int tmp;
        tmp = selection_start;
        selection_start = selection_end;
        selection_end = tmp;
    }

    selecting = false;

    if(selection_start != selection_end) {
        have_selection = true;
        selection.start = selection_start;
        selection.end = selection_end;
        selection.len = selection_end - selection_start + 1;

        emit selection_changed(selection_start + current_offset,
                               selection_end + current_offset);

        menu_selection->setEnabled(true);
        action_copy_selection->setEnabled(true);

        viewport()->repaint();
        return;
    }

    have_selection = false;
    menu_selection->setDisabled(true);
    action_copy_selection->setDisabled(true);
    selection.start = 0;
    selection.end = 0;
    selection.len = 0;
}

void rv_hex_edit::mouseMoveEvent(QMouseEvent *event)
{
    if(!have_data) return;
    const int x = event->x();
    const int y = event->y();
    if(selecting) {
        set_cursor_data(&tmp_cursor, x, y);
        if(selection_end == tmp_cursor.offset)
            return; // nothing changed

        selection_end = tmp_cursor.offset;
        viewport()->repaint();
    }

    set_shadow_cursor(x, y);
}

void rv_hex_edit::handle_click(int x, int y)
{
    old_cursor = cursor;
    set_cursor_data(&old_cursor, old_cursor.offset); // update to clear properly

    set_cursor_data(&cursor, x, y);

    if(cursor.offset != old_cursor.offset) {
        repaint_cursor(&old_cursor, &cursor);
        emit cursor_offset_changed_to(cursor.offset + current_offset);
    }
}

void rv_hex_edit::set_shadow_cursor(int x, int y)
{

    set_cursor_data(&shadow_cursor, x, y);
    set_cursor_data(&old_shadow_cursor, old_shadow_cursor.offset);

    if(shadow_cursor.offset != old_shadow_cursor.offset) {
        repaint_cursor(&old_shadow_cursor, &shadow_cursor);
        old_shadow_cursor = shadow_cursor;

        int tmp_single_range = single_range;

        if(!color_ranges.isEmpty()) {
            for(int i=0; i<color_ranges.count(); i++) {
                if((shadow_cursor.offset >=
                        color_ranges.at(i).start)
                        && (shadow_cursor.offset <=
                            color_ranges.at(i).end)) {
                    single_range = i;
                    if(single_range != tmp_single_range) {
                        viewport()->repaint();
                    }

                    return;
                }
            }
        }
    }
}

void rv_hex_edit::set_cursor_data(rv_hv_cursor *c, int x, int y)
{
    if( y <= (MARGIN_TOP + header_col_height)) return;


    // offs col clicked
    if(x < (MARGIN_LEFT + header_offs_width)) {
        return;
    }

    // asc col clicked
    if(x > (MARGIN_LEFT + header_offs_width
            + 16*cell_geometry.width)
            ) {
        return;
    }

    if(((y - MARGIN_TOP - header_col_height) / line_height ) >= num_lines) return;

    c->screen_row = (y - MARGIN_TOP- header_col_height) / line_height;
    c->row = c->screen_row + verticalScrollBar()->value();

    c->col = (x - MARGIN_LEFT - header_offs_width) / cell_geometry.width;

    c->offset = 16 * c->row + c->col;
    c->on_screen = true; // since we call from a click
    c->x = c->col * cell_geometry.width +
            header_offs_width + MARGIN_LEFT;
    c->y = c->screen_row * line_height + header_col_height + MARGIN_TOP;
}



void rv_hex_edit::set_cursor_data(rv_hv_cursor *c, unsigned int offs)
{
    unsigned int row = verticalScrollBar()->value();
    c->offset   = offs;
    c->col      = offs % 16;
    c->row      = offs / 16;

    if( (c->row >= row) && (c->row <= (row + num_lines)) ) {
        c->on_screen = true;
        c->x = c->col * cell_geometry.width +
                header_offs_width + MARGIN_LEFT;
        c->screen_row = c->row - row;
        c->y = c->screen_row * line_height + header_col_height + MARGIN_TOP;
    } else {
        c->on_screen = false;
    }
}

void rv_hex_edit::repaint_cursor(rv_hv_cursor *old_pos, rv_hv_cursor *new_pos)
{
        if(!have_data) return;
    // place new cursor

        QRegion r(new_pos->x, new_pos->y, cell_geometry.width, line_height);
        viewport()->repaint(r);

        // place new cursor in ascii
        QRegion r2(16 * cell_geometry.width + header_offs_width + MARGIN_LEFT,
                  new_pos->screen_row * line_height + header_col_height + MARGIN_TOP,
                  17*9, line_height);
        viewport()->repaint(r2);

        // set offset col of new cursor
        QRegion r4(MARGIN_LEFT,
                   MARGIN_TOP + header_col_height +  new_pos->screen_row * line_height,
                       header_offs_width - lmargin_col_ascii,
                       line_height);
        viewport()->repaint(r4);

        // set bytenum col of new cursor
        QRegion r6(MARGIN_LEFT + header_offs_width+ new_pos->col * cell_geometry.width,
                   MARGIN_TOP, cell_geometry.width - 1,
                   header_col_height - hdr_content_space - 1);
        viewport()->repaint(r6);



    // remove old cursor
        QRegion r1(old_pos->x, old_pos->y,cell_geometry.width, line_height);
        viewport()->repaint(r1);

        // remove old cursor in ascii
        QRegion r3(16 * cell_geometry.width + header_offs_width + MARGIN_LEFT,
                  old_pos->screen_row * line_height + header_col_height + MARGIN_TOP,
                  17*9, line_height);
        viewport()->repaint(r3);

        // remove offset col of old cursor
        QRegion r5(MARGIN_LEFT,
                   MARGIN_TOP + header_col_height +  old_pos->screen_row * line_height,
                       header_offs_width - lmargin_col_ascii,
                       line_height);
        viewport()->repaint(r5);


        // remove bytenum col of old cursor
        QRegion r7(MARGIN_LEFT + header_offs_width+ old_pos->col * cell_geometry.width,
                   MARGIN_TOP, cell_geometry.width - 1,
                   header_col_height - hdr_content_space - 1);
        viewport()->repaint(r7);
}

void rv_hex_edit::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(!have_data) return;
    if(cursor.offset >= current_data_len) return;

    if(editor) { editor->close(); editor=0; }
    editor = new QLineEdit(this);
    char buf[16];

    set_cursor_data(&cursor, event->x(), event->y());

    editor->move(cursor.x + 1, cursor.y + 1);
    editor->setFrame(false);
    editor->setContentsMargins(0,0,0,0);
    editor->setTextMargins(0,0,0,0);
    editor->setFixedWidth(cell_geometry.width);
    editor->setFixedHeight(line_height-1);
    editor->setMaxLength(2);
    editor->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    ::snprintf(buf, 3, "%02X", current_dataptr[cursor.offset]);

    editor->setText(QString::fromLatin1(buf));
    editor->selectAll();

    editor->show();
    editor->setFocus();
    connect(editor, SIGNAL(returnPressed()), this, SLOT(editor_enter_pressed()));
}

void rv_hex_edit::editor_enter_pressed()
{
    bool ok;
    QString tmp = editor->text();
    editor->close();

    unsigned int val = tmp.toUInt(&ok, 16);
    if(ok) {
        current_dataptr[cursor.offset] = (unsigned char) val;
        viewport()->repaint();    
        if(model) emit data_changed(model->index(0, 0),
                                    model->index(model->rowCount(),
                                                 model->columnCount()
                                                 ));
    }
}

void rv_hex_edit::set_single_range(QModelIndex i)
{
    if(i.row() >= color_ranges.count()) return;
    single_range = i.row();
    verticalScrollBar()->setValue(color_ranges.at(single_range).start / 16);
    viewport()->repaint();
}

void rv_hex_edit::set_single_range(int i)
{
    if(i >= color_ranges.count()) return;
    single_range = i;
    verticalScrollBar()->setValue(color_ranges.at(single_range).start / 16);
    viewport()->repaint();
}

void rv_hex_edit::set_cellgeometry(int w, int h, int bw, int bh, int tw, int th)
{
    cell_geometry.width = w;
    cell_geometry.height = h;
    line_height = h;
    cell_geometry.txt_width = tw;
    cell_geometry.txt_height = th;
    cell_geometry.bg_width = bw;
    cell_geometry.bg_height = bh;

    recalc_scrollbars();
}


void rv_hex_edit::paint_infoline(QPainter *painter)
{
    // closing whithe space above
    painter->fillRect(0, viewport()->height()-info_line_height - 5,
                     viewport()->width(),
                     5,
                     Qt::white);

    // info line
    painter->setPen(color_grid_info);
    painter->drawLine(0, viewport()->height()-info_line_height,
                     viewport()->width(), viewport()->height() - info_line_height);

    painter->fillRect(0, viewport()->height()- info_line_height + 1,
                     viewport()->width(),
                     info_line_height -1,
                     color_bg_info);

    painter->setPen(color_txt_info);
    painter->drawText(MARGIN_LEFT + 30, viewport()->height() -11,
                     info_txt);
}

void rv_hex_edit::paint_cell(QPainter *painter, int x, int y)
{
    char buf[16];
    unsigned int offs;
    unsigned int index_row = y + verticalScrollBar()->value();

    offs = x + index_row * 16;

    if(offs >= current_data_len) return;

    // paint cursor background
    if((x == cursor.col) && (index_row == cursor.row)) {
        painter->fillRect(MARGIN_LEFT + header_offs_width
                          + x * cell_geometry.width,
                          MARGIN_TOP + header_col_height
                          + y * cell_geometry.height,
                          cell_geometry.width -2,  // 1
                          cell_geometry.height-1,
                          color_cursor);
        painter->setPen(Qt::white);
        goto draw_text;
    }
//    else if((x == shadow_cursor.col) && (y == shadow_cursor.screen_row)) {
//        // shadow cursor!!
//        painter->fillRect(MARGIN_LEFT + header_offs_width
//                          + x * cell_geometry.width +1,
//                          MARGIN_TOP + header_col_height
//                          + y * cell_geometry.height +1,
//                          cell_geometry.width -1 -2,
//                          cell_geometry.height-1 -2,
//                          color_shadow_cursor);
//        painter->setPen(Qt::white);
//        goto draw_text;
//    }
    else if((have_selection || selecting)&& (((offs >= selection_start)
              && (offs <= selection_end)) ||
              ((offs >= selection_end)
                            && (offs <= selection_start)))
              ) {
        painter->fillRect(MARGIN_LEFT + header_offs_width
                          + x * cell_geometry.width  + 1 +1 ,
                          MARGIN_TOP + header_col_height
                          + y * cell_geometry.height + 1,
                          cell_geometry.width - 4 -2,
                          cell_geometry.height-3,
                          selection_bgcolor);
        painter->setPen(selection_txtcolor);
        goto draw_text;
    }

    else if(!color_ranges.isEmpty()) {
        // color_ranges
        /*if((single_range >= 0) && (color_ranges.count() > single_range)) {
            if((offs >= color_ranges.at(single_range).start)
                    && (offs <= color_ranges.at(single_range).end)
                    ) {
                painter->fillRect(MARGIN_LEFT + header_offs_width
                                  + x * cell_geometry.width  + 1 +1 ,
                                  MARGIN_TOP + header_col_height
                                  + y * cell_geometry.height + 1,
                                  cell_geometry.width - 4 -2,
                                  cell_geometry.height-3,
                                  color_ranges.at(single_range).bgcolor);
                painter->setPen(color_ranges.at(single_range).txtcolor);
                goto draw_text;
            }
        } else*/ {
            for(int i=0; i<color_ranges.count(); i++) {
                if((offs >= color_ranges.at(i).start)
                        && (offs <= color_ranges.at(i).end)
                        ) {
                    painter->fillRect(MARGIN_LEFT + header_offs_width
                                      + x * cell_geometry.width  + 1 +1 ,
                                      MARGIN_TOP + header_col_height
                                      + y * cell_geometry.height + 1,
                                      cell_geometry.width - 4 -2,
                                      cell_geometry.height-3,
                                      color_ranges.at(i).bgcolor);
                    painter->setPen(color_ranges.at(i).txtcolor);
                    goto draw_text;
                }
            }
        }
    }

    // paint regular cell
    if(index_row % 2) {
        painter->fillRect(MARGIN_LEFT + header_offs_width
                          + x * cell_geometry.width  + 1 +1 ,
                          MARGIN_TOP + header_col_height
                          + y * cell_geometry.height + 1,
                          cell_geometry.width - 4 -2,
                          cell_geometry.height-3,
                          color_bg1_vals);
    } else {
        painter->fillRect(MARGIN_LEFT + header_offs_width
                          + x * cell_geometry.width  + 1 +1,
                          MARGIN_TOP + header_col_height
                          + y * cell_geometry.height + 1,
                          cell_geometry.width - 4 -2,
                          cell_geometry.height-3,
                          color_bg2_vals
                          );
    }
    painter->setPen(color_hex_vals);

    draw_text:
        if((x == shadow_cursor.col) && (y == shadow_cursor.screen_row)) {
                // shadow cursor!!
                painter->fillRect(MARGIN_LEFT + header_offs_width
                                  + x * cell_geometry.width +1,
                                  MARGIN_TOP + header_col_height
                                  + y * cell_geometry.height +1,
                                  cell_geometry.width -1 -2-1,
                                  cell_geometry.height-1 -2,
                                  color_shadow_cursor);
                //painter->setPen(Qt::white);
    }

        ::snprintf(buf, 3, "%02X", current_dataptr[offs]);



    painter->drawText(MARGIN_LEFT + header_offs_width
                      + x * cell_geometry.width
                      + (cell_geometry.width -cell_geometry.txt_width)/2 + 5,
                      MARGIN_TOP + header_col_height + y * cell_geometry.height
                      + cell_geometry.height - 11,
                      QString::fromLatin1(buf));
}

void rv_hex_edit::paint_grid(QPainter *painter)
{
    painter->setPen(color_grid);
    painter->drawLine(MARGIN_LEFT + header_offs_width - 3,
                      MARGIN_TOP + header_col_height,
                      MARGIN_LEFT + header_offs_width - 3,
                      MARGIN_TOP + header_col_height + num_lines * line_height
                      );

    painter->drawLine(MARGIN_LEFT + header_offs_width + 16 * cell_geometry.width + 1,
                      MARGIN_TOP + header_col_height,
                      MARGIN_LEFT + header_offs_width + 16 * cell_geometry.width + 1,
                      MARGIN_TOP + header_col_height + num_lines * line_height
                      );

    painter->drawLine(MARGIN_LEFT + header_offs_width + 8 * cell_geometry.width - 1,
                      MARGIN_TOP + header_col_height,
                      MARGIN_LEFT + header_offs_width + 8 * cell_geometry.width - 1,
                      MARGIN_TOP + header_col_height + num_lines * line_height
                      );

   painter->drawLine(MARGIN_LEFT + header_offs_width + 16 * cell_geometry.width
           + lmargin_col_ascii + 2 + 16*9 + 2,
                     MARGIN_TOP,
                     MARGIN_LEFT + header_offs_width + 16 * cell_geometry.width
                                 + lmargin_col_ascii + 2 + 16*9 + 2,
                     viewport()->height() - MARGIN_TOP);
}

void rv_hex_edit::paint_range_desc(QPainter *painter, int i)
{
    painter->fillRect(MARGIN_LEFT, viewport()->height()-line_height + 2,
                     20,
                     line_height-6,
                     color_ranges.at(i).bgcolor);
    painter->setPen(color_txt_info);
    painter->drawText(MARGIN_LEFT + 30, viewport()->height() -11,
                     color_ranges.at(i).desc);
}

void rv_hex_edit::paint_hdr_col(QPainter *painter, QRect boundr)
{
    QString tmp;
    char buf[16];
    for(int i=0; i<16; i++) {
        if(boundr.intersects(QRect(MARGIN_LEFT + header_offs_width+ i * cell_geometry.width,
                                   MARGIN_TOP, cell_geometry.width - 1,
                                   header_col_height - hdr_content_space - 1
                                   ))) {
        if(cursor.col == i) {
            // cursor
            painter->fillRect(MARGIN_LEFT + header_offs_width
                              + i * cell_geometry.width,
                              MARGIN_TOP,
                              cell_geometry.width - 1,

                              header_col_height - hdr_content_space - 1,
                              color_bg_hdr_selected
                              );
            painter->setPen(Qt::white);
        } else if(shadow_cursor.col == i) {
            //shadow_cursor
            painter->fillRect(MARGIN_LEFT + header_offs_width
                              + i * cell_geometry.width,
                              MARGIN_TOP,
                              cell_geometry.width - 1,

                              header_col_height - hdr_content_space - 1,
                              color_shadow_bg_hdr_selected
                              );
            painter->setPen(Qt::white);
        } else {
            painter->fillRect(MARGIN_LEFT + header_offs_width
                              + i * cell_geometry.width,
                              MARGIN_TOP,
                              cell_geometry.width - 1,

                              header_col_height - hdr_content_space - 1,
                              color_bg_hdr
                              );
            painter->setPen(color_txt_hdr);
        }
        ::snprintf(buf, 3, "%02X", i);
        tmp = QString::fromLatin1(buf);
        painter->drawText(MARGIN_LEFT + header_offs_width
                          + i * cell_geometry.width
                          + (cell_geometry.width -cell_geometry.txt_width)/2 + 5,
                          MARGIN_TOP + header_col_height - hdr_content_space
                          - 9,
                          tmp);
    }
    }
}

void rv_hex_edit::paint_asc_col(QPainter *painter, int y)
{
    // draw ascii
    QString tmp = "";
    QChar c;
    char x;

    unsigned int index_row, offs;
    int font_width = 9;

    index_row = verticalScrollBar()->value() + y;

    int x1, y1;

    x1 = MARGIN_LEFT + header_offs_width + 16 * cell_geometry.width
            + lmargin_col_ascii + 2;
    y1 = MARGIN_TOP + header_col_height + y * cell_geometry.height;

    painter->fillRect(x1, y1, font_width * 16, line_height-3,
                      Qt::white);

    for(int cnt = 0; cnt < 16; cnt++) {
        offs = index_row * 16 + cnt;
        if(offs < current_data_len) {
            c = current_dataptr[offs];
            x = (char) current_dataptr[offs];
        } else  {
            c=' ';
            x = 0;
        }

        if( ((x >= '0') && (x<='9')) || ((x >= 'a') && (x<='z'))
               || ((x >= 'A') && (x<='Z')) ) {
            tmp=c;
            if(!(offs == cursor.offset))
                painter->setPen(color_hex_vals);
        } else if( ((x >= '!') && (x<='/')) ) {
               tmp=c;
               if(!(offs == cursor.offset))
                   painter->setPen(color_asc_vals);
           } else if(c.isPrint()) {
            tmp=c;
            if(!(offs == cursor.offset))
                painter->setPen(color_asc_vals2);
        } else  {
            tmp = ".";
            if(!(index_row *16 + cnt == cursor.offset))
                painter->setPen(color_grid);
        }

        if(offs == cursor.offset) {
            painter->fillRect(x1 + cnt*font_width,
                             y1+1,
                             font_width,
                             line_height -3,
                             color_cursor2
                             );
            painter->setPen(QColor(Qt::white));
        } else if(offs == shadow_cursor.offset) {
            painter->fillRect(x1 + cnt*font_width,
                             y1+1,
                             font_width,
                             line_height -3,
                             color_shadow_cursor
                             );
        } else if((have_selection || selecting) && (((offs >= selection_start)
                                                    && (offs <= selection_end)) ||
                                                    ((offs >= selection_end)
                                                                  && (offs <= selection_start)))
                                                    ) {
            painter->fillRect(x1 + cnt*font_width,
                             y1+1,
                             font_width,
                             line_height -3,
                             selection_bgcolor
                             );
            painter->setPen(selection_txtcolor);
        }

        painter->drawText(x1 + cnt*font_width, y1 + 1 + line_height -8, tmp);
    }
}

void rv_hex_edit::set_model(QAbstractItemModel *m)
{
    model = m;
}


// ---- GETTER FUNCTIONS ----
bool rv_hex_edit::get_selection(rv_hv_selection &sel)
{
    if(have_selection) {
        sel.start = selection_start;
        sel.end = selection_end;
        sel.hv = this;
        sel.len = selection_end - selection_start + 1;

        return true;
    }

    sel.start = 0;
    sel.end = 0;
    sel.hv = 0;

    return false;
}

unsigned char *rv_hex_edit::get_data_ptr()
{
    return current_dataptr;
}

unsigned int rv_hex_edit::get_current_offset()
{
    return current_offset;
}

unsigned int rv_hex_edit::get_cursor_offset()
{
    return cursor.offset;
}

unsigned int rv_hex_edit::get_data_len()
{
    return current_data_len;
}

// ---- SETTER FUNCTIONS ----
void rv_hex_edit::add_color_range(unsigned int start,
                                  unsigned int end,
                                  QColor bgcolor,
                                  QColor txtcolor,
                                  QString desc)
{
    struct rv_hv_color_range cr;
    cr.start = start;
    cr.end = end;
    cr.bgcolor = bgcolor;
    cr.txtcolor = txtcolor;
    cr.desc = desc;

    color_ranges.prepend(cr);

    have_color_ranges = true;
}

void rv_hex_edit::add_color_range(rv_hv_color_range cr)
{
    color_ranges.prepend(cr);

    have_color_ranges = true;
}

void rv_hex_edit::set_data_buffer(unsigned char *buf, unsigned int len)
{
    original_dataptr = buf;
    original_len = len;

    current_dataptr = original_dataptr;
    current_offset = 0;
    current_data_len = original_len;

    have_data = true;

    color_bg1_vals  = QColor(0xe8, 0xf0, 0xf8);
    color_bg2_vals  = QColor(0xd8, 0xe0, 0xe8);
    recalc_scrollbars();
}

void rv_hex_edit::set_buffer_name(QString name)
{
    buffer_name = name;
}

QString rv_hex_edit::get_name()
{
    return buffer_name;
}

bool rv_hex_edit::paste_foreign_selection(rv_hv_selection p_sel)
{
    if(!p_sel.hv) return false;

    unsigned char *from;
    from = p_sel.hv->get_data_ptr();

    unsigned int paste_len = p_sel.len;

    if((cursor.offset +  paste_len) > current_data_len) {
        paste_len = current_data_len - cursor.offset;
    }

    for(int i=0; i < paste_len; i++)
        current_dataptr[cursor.offset + i] = from[i + p_sel.start];

    viewport()->repaint();

    return true;
}

void rv_hex_edit::setup_actions()
{
    action_undo = undo_stack->createUndoAction(this, tr("&Undo"));
    action_redo = undo_stack->createRedoAction(this, tr("&Redo"));

    action_undo->setIcon(QIcon(":/icons/undo3.png"));
    action_redo->setIcon(QIcon(":/icons/redo3.png"));

    action_copy_selection   = new QAction(QIcon(":/icons/edit-copy.png"),
                                        "&Copy selection", this);

    action_paste_selection  = new QAction(QIcon(":/icons/edit-paste.png"),
                                        "&Paste", this);

    action_fill_selection   = new QAction(QIcon(":/icons/edit-fill.png"),
                                        "&Fill selection", this);

    action_edit_at_cursor   = new QAction("&Edit at current cursor", this);

    connect(action_copy_selection, SIGNAL(triggered()),
            this, SLOT(ar_copy_selection()));

    connect(action_paste_selection, SIGNAL(triggered()),
            this, SLOT(ar_paste()));

    connect(action_fill_selection, SIGNAL(triggered()),
            this, SLOT(ar_fill_selection()));

}

void rv_hex_edit::setup_menus()
{
    context_menu = new QMenu("HEX Tools", this);
    context_menu->addAction(action_undo);
    context_menu->addAction(action_redo);
    context_menu->addSeparator();

    context_menu->addAction(action_copy_selection);
    context_menu->addAction(action_paste_selection);
    context_menu->addSeparator();

    menu_selection = new QMenu("Selection", this);
    menu_selection->addAction(action_fill_selection);
    context_menu->addMenu(menu_selection);
    context_menu->addSeparator();

    context_menu->addAction(action_edit_at_cursor);

    menu_selection->setDisabled(true);
    action_copy_selection->setDisabled(true);
}

void rv_hex_edit::contextMenuEvent(QContextMenuEvent *event) {
    context_menu->exec(event->globalPos());
}


void rv_hex_edit::ar_copy_selection()
{
    if(!have_selection) return;
    QUndoCommand *cmd = new uc_copy_selection(selection);
    undo_stack->push(cmd);

    //viewport()->repaint();
}

void rv_hex_edit::ar_paste()
{
    QUndoCommand *cmd = new uc_paste(selection_paste, cursor.offset);
    undo_stack->push(cmd);
}

void rv_hex_edit::ar_fill_selection()
{

}

void rv_hex_edit::set_can_paste(bool yesno)
{
    can_paste = yesno;
    action_paste_selection->setEnabled(yesno);
}

bool rv_hex_edit::get_can_paste()
{
    return can_paste;
}

void rv_hex_edit::set_selection(rv_hv_selection sel)
{
    selection_start = sel.start;
    selection_end = sel.end;
    handle_selection();
}

void rv_hex_edit::do_repaint()
{
    viewport()->repaint();
}

void rv_hex_edit::set_shadow_cursor(unsigned int offs)
{
    unsigned int goto_row;

    set_cursor_data(&shadow_cursor, offs);
    if(shadow_cursor.row <= (num_lines/2)) goto_row = 0;
    else goto_row = shadow_cursor.row - (num_lines/2);

    verticalScrollBar()->setValue(goto_row);
    set_cursor_data(&shadow_cursor, offs);
    viewport()->repaint();
}

void rv_hex_edit::set_cursor_to_offs(unsigned int offs)
{
    unsigned int goto_row;

    set_cursor_data(&cursor, offs);
    if(cursor.row <= (num_lines/2)) goto_row = 0;
    else goto_row = cursor.row - (num_lines/2);

    verticalScrollBar()->setValue(goto_row);
    set_cursor_data(&cursor, offs);
    viewport()->repaint();
}
