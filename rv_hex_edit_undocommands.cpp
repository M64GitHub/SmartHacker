#include "rv_hex_edit_undocommands.h"

// ---- COPY ----

uc_copy_selection::uc_copy_selection(rv_hv_selection sel, QUndoCommand *parent)
    : QUndoCommand(parent)
 {
    my_sel = sel;
    my_copy_buffer = (unsigned char *) malloc(sel.len);

    old_can_paste = sel.hv->get_can_paste();
    old_paste_selection = sel.hv->selection_paste;
    old_paste_buffer = sel.hv->selection_paste_data;

    unsigned char *from = my_sel.hv->get_data_ptr();

    for(int i=0; i < my_sel.len; i++)
        my_copy_buffer[i] = from[i + my_sel.start];

    setText("Copy [" + QString::number(sel.start, 16) + "-"
            + QString::number(sel.end, 16) + "]");
 }

uc_copy_selection::~uc_copy_selection()
{
    if(my_copy_buffer) free(my_copy_buffer);
}

void uc_copy_selection::redo()
{
    my_sel.hv->set_can_paste(true);
    my_sel.hv->selection_paste = my_sel;
    my_sel.hv->selection_paste_data = my_copy_buffer;
    my_sel.hv->set_selection(my_sel);
}

void uc_copy_selection::undo()
{
    my_sel.hv->set_can_paste(old_can_paste);
    my_sel.hv->selection_paste = old_paste_selection;
    my_sel.hv->selection_paste_data = old_paste_buffer;
    my_sel.hv->set_selection(my_sel);
}


// ---- PASTE ----

uc_paste::uc_paste(rv_hv_selection sel, unsigned int to, QUndoCommand *parent)
    : QUndoCommand(parent)
 {
    my_sel = sel;
    to_offset = to;
    if(!my_sel.hv->get_selection(old_sel)) {
        old_sel.start = 0;
        old_sel.end = 0;
    }

    unsigned int len;

    len = my_sel.len;

    if((to + len) >= my_sel.hv->get_data_len())
        my_sel.len = my_sel.hv->get_data_len() - to;

    undo_buf = (unsigned char *) malloc(my_sel.len);

    unsigned char *from = my_sel.hv->get_data_ptr();

    for(int i=0; i < my_sel.len; i++)
        undo_buf[i] = from[i + to_offset];

    setText("Paste to " + QString::number(to_offset, 16)
            + " [" + QString::number(sel.start, 16) + "-"
            + QString::number(sel.end, 16) + "]" + " ("
            + QString::number(sel.len) + ") bytes");
 }

void uc_paste::redo()
{
    unsigned char *to = my_sel.hv->get_data_ptr();
    unsigned char *from = my_sel.hv->selection_paste_data;

    for(int i=0; i < my_sel.len; i++)
        to[i + to_offset] = from[i];

    // my_sel.hv->do_repaint();
    struct rv_hv_selection tmp_sel;
    tmp_sel.start = to_offset;
    tmp_sel.end = to_offset + my_sel.len;
    tmp_sel.len = my_sel.len;
    tmp_sel.hv = my_sel.hv;
    my_sel.hv->set_selection(tmp_sel);
}

void uc_paste::undo()
{
    unsigned char *to = my_sel.hv->get_data_ptr();
    unsigned char *from = undo_buf;

    for(int i=0; i < my_sel.len; i++)
        to[i + to_offset] = from[i];

    my_sel.hv->set_selection(old_sel);
}



// ---- FILL ----

uc_fill_selection::uc_fill_selection(rv_hv_selection sel, QUndoCommand *parent)
    : QUndoCommand(parent)
 {
 }

void uc_fill_selection::redo()
{

}

void uc_fill_selection::undo()
{

}
