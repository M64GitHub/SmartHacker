#ifndef RV_HEX_EDIT_UNDOCOMMAND_H
#define RV_HEX_EDIT_UNDOCOMMAND_H

#include <QUndoCommand>
#include <rv_hex_edit.h>

class uc_copy_selection : public QUndoCommand
 {
 public:
     uc_copy_selection(struct rv_hv_selection sel, QUndoCommand *parent = 0);
     ~uc_copy_selection();

     void undo();
     void redo();

 private:
    struct rv_hv_selection my_sel;
    struct rv_hv_selection old_paste_selection;
    bool old_can_paste;
    unsigned char *old_paste_buffer;
    unsigned char *my_copy_buffer;
 };

class uc_paste : public QUndoCommand
 {
 public:
     uc_paste(struct rv_hv_selection sel, unsigned int to, QUndoCommand *parent = 0);

     void undo();
     void redo();

 private:
     struct rv_hv_selection my_sel;
     unsigned char *undo_buf;
     unsigned int to_offset;
     struct rv_hv_selection old_sel;
 };

class uc_fill_selection : public QUndoCommand
 {
 public:
     uc_fill_selection(struct rv_hv_selection sel, QUndoCommand *parent = 0);

     void undo();
     void redo();

 private:
     unsigned char *undo_buf;
 };

#endif // RV_HEX_EDIT_UNDOCOMMAND_H
