/********************************************************************\
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/*
 * The Gnucash Register widget
 *
 *  Based heavily on the Gnumeric Sheet widget.
 *
 * Authors:
 *     Heath Martin <martinh@pegasus.cc.ucf.edu>
 *     Dave Peticolas <dave@krondo.com>
 */

#include "config.h"
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>

#include "gnucash-register.h"
#include "gnucash-sheet.h"
#include "gnucash-sheetP.h"

#include "gnucash-cursor.h"
#include "gnucash-style.h"
#include "gnucash-header.h"
#include "gnucash-item-edit.h"
#include "split-register.h"
#include "gnc-engine.h"         // For debugging, e.g. ENTER(), LEAVE()
#include "gnc-prefs.h"
#include "gnc-state.h"


/* Register signals */
enum
{
    ACTIVATE_CURSOR,
    REDRAW_ALL,
    REDRAW_HELP,
    LAST_SIGNAL
};


/** Static Globals *****************************************************/

/* This static indicates the debugging module that this .o belongs to. */
static QofLogModule log_module = GNC_MOD_REGISTER;
static GtkTable *register_parent_class;
static guint register_signals[LAST_SIGNAL];


struct _GnucashRegister
{
    GtkTable table;

    GtkWidget *hscrollbar;
    GtkWidget *sheet;
    gboolean  hscrollbar_visible;
};


struct _GnucashRegisterClass
{
    GtkTableClass parent_class;

    void (*activate_cursor) (GnucashRegister *reg);
    void (*redraw_all)      (GnucashRegister *reg);
    void (*redraw_help)     (GnucashRegister *reg);
};

/** Implementation *****************************************************/

gboolean
gnucash_register_has_selection (GnucashRegister *reg)
{
    GnucashSheet *sheet;
    GncItemEdit *item_edit;

    g_return_val_if_fail((reg != NULL), FALSE);
    g_return_val_if_fail(GNUCASH_IS_REGISTER(reg), FALSE);

    sheet = GNUCASH_SHEET(reg->sheet);
    item_edit = GNC_ITEM_EDIT(sheet->item_editor);

    return gnc_item_edit_get_has_selection(item_edit);
}

void
gnucash_register_cut_clipboard (GnucashRegister *reg)
{
    GnucashSheet *sheet;
    GncItemEdit *item_edit;

    g_return_if_fail(reg != NULL);
    g_return_if_fail(GNUCASH_IS_REGISTER(reg));

    sheet = GNUCASH_SHEET(reg->sheet);
    item_edit = GNC_ITEM_EDIT(sheet->item_editor);

    gnc_item_edit_cut_clipboard(item_edit);
}

void
gnucash_register_copy_clipboard (GnucashRegister *reg)
{
    GnucashSheet *sheet;
    GncItemEdit *item_edit;

    g_return_if_fail(reg != NULL);
    g_return_if_fail(GNUCASH_IS_REGISTER(reg));

    sheet = GNUCASH_SHEET(reg->sheet);
    item_edit = GNC_ITEM_EDIT(sheet->item_editor);

    gnc_item_edit_copy_clipboard(item_edit);
}

void
gnucash_register_paste_clipboard (GnucashRegister *reg)
{
    GnucashSheet *sheet;
    GncItemEdit *item_edit;

    g_return_if_fail(reg != NULL);
    g_return_if_fail(GNUCASH_IS_REGISTER(reg));

    sheet = GNUCASH_SHEET(reg->sheet);
    item_edit = GNC_ITEM_EDIT(sheet->item_editor);

    gnc_item_edit_paste_clipboard (item_edit);
}

void
gnucash_register_refresh_from_prefs (GnucashRegister *reg)
{
    GnucashSheet *sheet;

    g_return_if_fail(reg != NULL);
    g_return_if_fail(GNUCASH_IS_REGISTER(reg));

    sheet = GNUCASH_SHEET(reg->sheet);
    gnucash_sheet_refresh_from_prefs(sheet);
}


void
gnucash_register_goto_virt_cell (GnucashRegister *reg,
                                 VirtualCellLocation vcell_loc)
{
    GnucashSheet *sheet;
    VirtualLocation virt_loc;

    g_return_if_fail(reg != NULL);
    g_return_if_fail(GNUCASH_IS_REGISTER(reg));

    sheet = GNUCASH_SHEET(reg->sheet);

    virt_loc.vcell_loc = vcell_loc;
    virt_loc.phys_row_offset = 0;
    virt_loc.phys_col_offset = 0;

    gnucash_sheet_goto_virt_loc(sheet, virt_loc);
}

void
gnucash_register_goto_virt_loc (GnucashRegister *reg,
                                VirtualLocation virt_loc)
{
    GnucashSheet *sheet;

    g_return_if_fail(reg != NULL);
    g_return_if_fail(GNUCASH_IS_REGISTER(reg));

    sheet = GNUCASH_SHEET(reg->sheet);

    gnucash_sheet_goto_virt_loc(sheet, virt_loc);
}

void
gnucash_register_goto_next_virt_row (GnucashRegister *reg)
{
    GnucashSheet *sheet;
    VirtualLocation virt_loc;
    int start_virt_row;

    g_return_if_fail (reg != NULL);
    g_return_if_fail (GNUCASH_IS_REGISTER(reg));

    sheet = GNUCASH_SHEET(reg->sheet);

    gnucash_cursor_get_virt (GNUCASH_CURSOR(sheet->cursor), &virt_loc);

    /* Move down one physical row at a time until we
     * reach the next visible virtual cell. */
    start_virt_row = virt_loc.vcell_loc.virt_row;
    do
    {
        if (!gnc_table_move_vertical_position (sheet->table, &virt_loc, 1))
            return;
    }
    while (start_virt_row == virt_loc.vcell_loc.virt_row);

    if (virt_loc.vcell_loc.virt_row >= sheet->num_virt_rows)
        return;

    virt_loc.phys_row_offset = 0;
    virt_loc.phys_col_offset = 0;

    gnucash_sheet_goto_virt_loc (sheet, virt_loc);
}

void
gnucash_register_goto_next_matching_row (GnucashRegister *reg,
        VirtualLocationMatchFunc match,
        gpointer user_data)
{
    GnucashSheet *sheet;
    SheetBlockStyle *style;
    VirtualLocation virt_loc;

    g_return_if_fail (reg != NULL);
    g_return_if_fail (GNUCASH_IS_REGISTER(reg));
    g_return_if_fail (match != NULL);

    sheet = GNUCASH_SHEET (reg->sheet);

    gnucash_cursor_get_virt (GNUCASH_CURSOR(sheet->cursor), &virt_loc);

    do
    {
        if (!gnc_table_move_vertical_position (sheet->table,
                                               &virt_loc, 1))
            return;

        if (virt_loc.vcell_loc.virt_row >= sheet->num_virt_rows)
            return;

        style = gnucash_sheet_get_style (sheet, virt_loc.vcell_loc);
        if (!style || !style->cursor)
            return;
    }
    while (!match (virt_loc, user_data));

    virt_loc.phys_row_offset = 0;
    virt_loc.phys_col_offset = 0;

    gnucash_sheet_goto_virt_loc (sheet, virt_loc);
}


static void
gnucash_register_update_hadjustment (GtkAdjustment *adj,
                                     GnucashRegister *reg)
{
    g_return_if_fail (reg != NULL);
    g_return_if_fail (GNUCASH_IS_REGISTER(reg));

    if (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj)
        > gtk_adjustment_get_page_size (adj))
    {
        if (!reg->hscrollbar_visible)
        {
            gtk_widget_show(reg->hscrollbar);
            reg->hscrollbar_visible = TRUE;
        }
    }
    else
    {
        if (reg->hscrollbar_visible)
        {
            gtk_widget_hide(reg->hscrollbar);
            reg->hscrollbar_visible = FALSE;
        }
    }
}

/*************************************************************/


static void
gnucash_register_class_init (GnucashRegisterClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);

    register_parent_class = g_type_class_peek_parent (klass);

    register_signals[ACTIVATE_CURSOR] =
        g_signal_new("activate_cursor",
                     G_TYPE_FROM_CLASS(gobject_class),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GnucashRegisterClass,
                                     activate_cursor),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    register_signals[REDRAW_ALL] =
        g_signal_new("redraw_all",
                     G_TYPE_FROM_CLASS(gobject_class),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GnucashRegisterClass,
                                     redraw_all),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    register_signals[REDRAW_HELP] =
        g_signal_new("redraw_help",
                     G_TYPE_FROM_CLASS(gobject_class),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GnucashRegisterClass,
                                     redraw_help),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    klass->activate_cursor = NULL;
    klass->redraw_all = NULL;
    klass->redraw_help = NULL;
}


static void
gnucash_register_init (GnucashRegister *g_reg)
{
    GtkTable *table = GTK_TABLE(g_reg);

    gtk_widget_set_can_focus (GTK_WIDGET(table), FALSE);
    gtk_widget_set_can_default (GTK_WIDGET(table), FALSE);

    gtk_table_set_homogeneous (table, FALSE);
    gtk_table_resize (table, 3, 2);
}


GType
gnucash_register_get_type (void)
{
    static GType gnucash_register_type = 0;

    if (!gnucash_register_type)
    {
        static const GTypeInfo gnucash_register_info =
        {
            sizeof (GnucashRegisterClass),
            NULL,		/* base_init */
            NULL,		/* base_finalize */
            (GClassInitFunc) gnucash_register_class_init,
            NULL,		/* class_finalize */
            NULL,		/* class_data */
            sizeof (GnucashRegister),
            0,		/* n_preallocs */
            (GInstanceInitFunc) gnucash_register_init,
        };

        gnucash_register_type = g_type_register_static
                                (gtk_table_get_type (),
                                 "GnucashRegister",
                                 &gnucash_register_info, 0);
    }

    return gnucash_register_type;
}


void
gnucash_register_attach_popup (GnucashRegister *reg,
                               GtkWidget *popup,
                               gpointer data)
{
    g_return_if_fail (GNUCASH_IS_REGISTER(reg));
    g_return_if_fail (reg->sheet != NULL);
    if (popup)
        g_return_if_fail (GTK_IS_WIDGET(popup));

    gnucash_sheet_set_popup (GNUCASH_SHEET (reg->sheet), popup, data);
}


/* Um, this function checks that data is not null but never uses it.
 *  Weird.  Also, since this function only works with a GnucashRegister
 *  widget, maybe some of it should be moved to gnucash-sheet.c. */
/* Adding to previous note:  Since data doesn't appear do anything and to
 *  align the function with save_state() I've removed the check for
 *  NULL and changed two calls in dialog_order.c and dialog_invoice.c
 *  to pass NULL as second parameter. */

static void
gnucash_register_configure (GnucashSheet *sheet, gchar * state_section)
{
    GNCHeaderWidths widths;
    GnucashRegister *greg;
    Table *table;
    GList *node;
    gchar *key;
    guint value;
    GKeyFile *state_file = gnc_state_get_current();

    // Stuff for per-register settings load.
    g_return_if_fail (sheet != NULL);
    g_return_if_fail (GNUCASH_IS_SHEET (sheet));

    PINFO("state_section=%s",state_section);

    ENTER("sheet=%p, data=%p", sheet, "");

    table = sheet->table;
    gnc_table_init_gui (table);
    table->ui_data = sheet;

    g_object_ref (sheet);

    /* config the cell-block styles */

    widths = gnc_header_widths_new ();

    if (state_section && gnc_prefs_get_bool(GNC_PREFS_GROUP_GENERAL, GNC_PREF_SAVE_GEOMETRY))
    {
        node = gnc_table_layout_get_cells (table->layout);
        for (; node; node = node->next)
        {
            BasicCell *cell = node->data;

            if (cell->expandable)
                continue;

            /* Remember whether the column is visible */
            key = g_strdup_printf("%s_width", cell->cell_name);
            value = g_key_file_get_integer (state_file, state_section, key, NULL);
            if (value != 0)
                gnc_header_widths_set_width (widths, cell->cell_name, value);
            g_free(key);
        }
    }

    gnucash_sheet_create_styles (sheet);

    gnucash_sheet_set_header_widths (sheet, widths);

    gnucash_sheet_compile_styles (sheet);

    gnucash_sheet_table_load (sheet, TRUE);
    gnucash_sheet_cursor_set_from_table (sheet, TRUE);
    gnucash_sheet_redraw_all (sheet);

    gnc_header_widths_destroy (widths);

    LEAVE(" ");
}


static GtkWidget *
gnucash_register_create_widget (Table *table)
{
    GnucashRegister *reg;
    GtkWidget *header;
    GtkWidget *widget;
    GtkWidget *sheet;
    GtkWidget *scrollbar;
    GtkWidget *box;

    reg = g_object_new (GNUCASH_TYPE_REGISTER, NULL);
    widget = GTK_WIDGET(reg);

    sheet = gnucash_sheet_new (table);
    reg->sheet = sheet;
    GNUCASH_SHEET(sheet)->reg = widget;

    header = gnc_header_new (GNUCASH_SHEET(sheet));

    gtk_table_attach (GTK_TABLE(widget), header,
                      0, 1, 0, 1,
                      GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                      GTK_FILL,
                      0, 0);
    gtk_widget_show(header);

    gtk_table_attach (GTK_TABLE(widget), sheet,
                      0, 1, 1, 2,
                      GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                      GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                      0, 0);
    gtk_widget_show(sheet);

    scrollbar = gtk_vscrollbar_new(GNUCASH_SHEET(sheet)->vadj);
    gtk_table_attach (GTK_TABLE(widget), GTK_WIDGET(scrollbar),
                      1, 2, 0, 3,
                      GTK_FILL,
                      GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                      0, 0);
    gtk_widget_show(scrollbar);

    scrollbar = gtk_hscrollbar_new(GNUCASH_SHEET(sheet)->hadj);
    gtk_table_attach (GTK_TABLE(widget), GTK_WIDGET(scrollbar),
                      0, 1, 2, 3,
                      GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                      GTK_FILL,
                      0, 0);
    reg->hscrollbar = scrollbar;
    gtk_widget_show(scrollbar);
    reg->hscrollbar_visible = TRUE;

    /* The gtkrc color helper widgets need to be part of a window
     * hierarchy so they can be realized. Stick them in a box
     * underneath the register, but don't show the box to the
     * user. */
    box = gtk_hbox_new(FALSE, 0);
    gtk_widget_set_no_show_all(GTK_WIDGET(box), TRUE);
    gtk_box_pack_start(GTK_BOX(box),
                                GNUCASH_SHEET(sheet)->header_color, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box),
                                GNUCASH_SHEET(sheet)->primary_color, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box),
                                GNUCASH_SHEET(sheet)->secondary_color, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box),
                                GNUCASH_SHEET(sheet)->split_color, TRUE, TRUE, 0);

    gtk_table_attach (GTK_TABLE(widget), box,
                      0, 1, 4, 5,
                      GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                      GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                      0, 0);

    g_signal_connect (GNUCASH_SHEET(sheet)->hadj, "changed",
                      G_CALLBACK (gnucash_register_update_hadjustment), reg);

    return widget;
}


GtkWidget *
gnucash_register_new (Table *table, gchar *state_section)
{
    GnucashRegister *reg;
    GtkWidget *header;
    GtkWidget *widget;
    GtkWidget *sheet;
    GtkWidget *scrollbar;
    GtkWidget *box;

    widget = gnucash_register_create_widget(table);
    reg = GNUCASH_REGISTER(widget);

    gnucash_register_configure (GNUCASH_SHEET(reg->sheet), state_section);

    return widget;
}


void gnucash_register_set_moved_cb (GnucashRegister *reg,
                                    GFunc cb, gpointer cb_data)
{
    GnucashSheet *sheet;

    if (!reg || !reg->sheet)
        return;
    sheet = GNUCASH_SHEET(reg->sheet);
    sheet->moved_cb = cb;
    sheet->moved_cb_data = cb_data;
}


GnucashSheet *gnucash_register_get_sheet (GnucashRegister *reg)
{
    g_return_val_if_fail (reg != NULL, NULL);
    g_return_val_if_fail (GNUCASH_IS_REGISTER(reg), NULL);

    return GNUCASH_SHEET(reg->sheet);
}

