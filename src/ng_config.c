#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
  int    show_label;
  int    show_background;
  int    orient;
  double size;
  double zoomfactor;
  int    autohide;
  int    autohide_show_urgent;
  int    lock_deskswitch;
  int    ecomorph_features;

  double hide_timeout;
  double zoom_duration;
  double zoom_range;
  int    zoom_one;

  int    hide_below_windows;
  int    hide_mode;
  double alpha;
  int    sia_remove;
  int stacking;
  int mouse_over_anim;

  Eina_List *boxes;

  Evas_Object *ilist;

  Evas_Object *tlist_box;
  Config_Box  *cfg_box;
  Config_Item *cfg;
  E_Config_Dialog *cfd;

  char *app_dir;
};

/* Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);


static void _cb_add(void *data, void *data2);
static void _cb_del(void *data, void *data2);
static void _cb_config(void *data, void *data2);
static void _cb_entry_ok(void *data, char *text);
static void _cb_confirm_dialog_yes(void *data);
static void _load_ilist(E_Config_Dialog_Data *cfdata);
/* static void _show_label_cb_change(void *data, Evas_Object *obj); */
static void _cb_slider_change(void *data, Evas_Object *obj);
/* static void _cb_check_if_launcher_source(void *data, Evas_Object *obj); */

static void _cb_box_add_launcher(void *data, void *data2);
static void _cb_box_add_taskbar(void *data, void *data2);
static void _cb_box_add_gadcon(void *data, void *data2);
static void _cb_box_del(void *data, void *data2);
static void _cb_box_config(void *data, void *data2);
static void _cb_box_up(void *data, void *data2);
static void _cb_box_down(void *data, void *data2);
static void _load_box_tlist(E_Config_Dialog_Data *cfdata);



void
ngi_configure_module(Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   Eina_List *l;
   char buf[4096];
   char path[128];
   Config_Item *ci2;
   int i = 0;
   
   if (ci->config_dialog) return;

   EINA_LIST_FOREACH(ngi_config->items, l, ci2)
     if (ci == ci2)
       break;
     else i++;
   
   snprintf(path, sizeof(path), "extensions/itask_ng::%d", i);
   if (e_config_dialog_find("E", path)) return;

   v = E_NEW(E_Config_Dialog_View, 1);

   /* Dialog Methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;

   /* Create The Dialog */
   snprintf(buf, sizeof(buf), "%s/e-module-ng.edj", e_module_dir_get(ngi_config->module));
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
			     D_("Itask NG Configuration"),
			     "E", path, buf, 0, v, ci);

   ci->config_dialog = cfd;
}



static void
_fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata)
{
   cfdata->show_background    = ci->show_background;
   cfdata->show_label         = ci->show_label;
   cfdata->orient             = ci->orient;
   cfdata->size               = ci->size;
   cfdata->zoomfactor         = ci->zoomfactor;
   cfdata->autohide           = ci->autohide;
   cfdata->autohide_show_urgent = ci->autohide_show_urgent;
   cfdata->hide_below_windows = ci->hide_below_windows;
   cfdata->hide_timeout       = ci->hide_timeout;
   cfdata->zoom_duration      = ci->zoom_duration;
   cfdata->zoom_range         = ci->zoom_range;
   cfdata->zoom_one           = ci->zoom_one;
   cfdata->alpha              = ci->alpha;
   cfdata->sia_remove         = ci->sia_remove;
   cfdata->stacking           = ci->stacking;
   cfdata->mouse_over_anim    = ci->mouse_over_anim;
   cfdata->lock_deskswitch    = ci->lock_deskswitch;
   cfdata->ecomorph_features  = ci->ecomorph_features;

   cfdata->cfg = ci;
   cfdata->cfd = ci->config_dialog;

   cfdata->ilist = NULL;
   cfdata->tlist_box = NULL;
}



static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;

   ci = (Config_Item*) cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(ci, cfdata);
   return cfdata;
}



static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   cfdata->cfg->config_dialog = NULL;
   free(cfdata);
}



static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   E_Radio_Group *rg;
   Evas_Object *o, *ol, *of, *ob, *ot, *o_table, *o_all;


   o_all = e_widget_list_add(evas, 0, 0);

   o_table = e_widget_table_add(evas, 0);

   /* _______ first column __________________________________________________*/
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_frametable_add(evas, D_("Bar Items"), 0);


   ol = e_widget_ilist_add(evas, 0, 0, NULL);
   cfdata->ilist = ol;
   _load_box_tlist(cfdata);
   e_widget_size_min_set(ol, 130, 100);
   e_widget_frametable_object_append(of, ol, 0, 0, 1, 1, 1, 1, 1, 1);

   ot = e_widget_table_add(evas, 0);
   ob = e_widget_button_add(evas, D_("Delete"), "widget/del", _cb_box_del, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 1, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, D_("Configure"), "widget/config", _cb_box_config, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, D_("Up"), "widget/up_arrow", _cb_box_up, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 1, 0, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, D_("Down"), "widget/down_arrow", _cb_box_down, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 1, 1, 1, 1, 1, 1, 1, 0);
   e_widget_frametable_object_append(of, ot, 0, 1, 1, 1, 1, 1, 1, 0);

   ot = e_widget_table_add(evas, 0);
   ob = e_widget_label_add(evas, D_("Add"));
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, D_("Taskbar"), "widget/add", _cb_box_add_taskbar, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 1, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, D_("Launcher"), "widget/add", _cb_box_add_launcher, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 1, 1, 1, 1, 1, 1, 1, 0);
   ob = e_widget_button_add(evas, D_("Gadcon"), "widget/add", _cb_box_add_gadcon, cfdata, NULL);
   e_widget_table_object_append(ot, ob, 0, 2, 1, 1, 1, 1, 1, 0);
   e_widget_frametable_object_append(of, ot, 0, 2, 1, 1, 1, 1, 1, 0);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_widget_table_object_append(o_table, o, 0, 0, 1, 1, 1, 1, 1, 1);

   /* _______ second column _________________________________________________*/
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, D_("Appearance"), 0);
   ob = e_widget_check_add(evas, D_("Show Icon Label"), &(cfdata->show_label));
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, D_("Show Background Box"), &(cfdata->show_background));
   e_widget_framelist_object_append(of, ob);
   /*
     ob = e_widget_check_add(evas, D_("Mouse-Over Animation"), &(cfdata->mouse_over_anim));
     e_widget_framelist_object_append(of, ob);
   */
   ob = e_widget_label_add (evas, D_("Icon Size:"));
   e_widget_framelist_object_append (of, ob);
   ob = e_widget_slider_add (evas, 1, 0, D_("%1.0f px"), 16.0, 128,
			     1.0, 0, &(cfdata->size), NULL, 100);
   e_widget_on_change_hook_set(ob, _cb_slider_change, cfdata);
   e_widget_framelist_object_append (of, ob);

   if (ngi_config->use_composite)
     {
	ob = e_widget_label_add (evas, D_("Background Transparency:"));
	e_widget_framelist_object_append (of, ob);
	ob = e_widget_slider_add (evas, 1, 0, D_("%1.0f \%"), 0, 255,
				  1.0, 0, &(cfdata->alpha), NULL, 100);
	e_widget_on_change_hook_set(ob, _cb_slider_change, cfdata);
	e_widget_framelist_object_append (of, ob);
     }
   else cfdata->alpha = 255;
   /*
     ob = e_widget_check_add(evas, D_("Below Fullscreen Windows"), &(cfdata->below_fullscreen));
     e_widget_framelist_object_append(of, ob);

     ob = e_widget_check_add(evas, D_("Draw On Desk"), &(cfdata->on_desk));
     e_widget_framelist_object_append(of, ob);
   */
   ob = e_widget_label_add(evas, D_("Stacking"));
   e_widget_framelist_object_append(of, ob);
   rg = e_widget_radio_group_new(&cfdata->stacking);
   ob = e_widget_radio_add(evas, D_("Above All"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, D_("Below Fullscreen"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, D_("On Desktop"), 2, rg);
   e_widget_framelist_object_append(of, ob);


   e_widget_list_object_append(o, of, 1, 1, 0.5);


   of = e_widget_frametable_add(evas, D_("Orientation"), 1);
   rg = e_widget_radio_group_new(&(cfdata->orient));
   ob = e_widget_radio_icon_add(evas, NULL, "enlightenment/shelf_position_left", 24, 24, E_GADCON_ORIENT_LEFT, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_icon_add(evas, NULL, "enlightenment/shelf_position_right", 24, 24, E_GADCON_ORIENT_RIGHT, rg);
   e_widget_frametable_object_append(of, ob, 2, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_icon_add(evas, NULL, "enlightenment/shelf_position_top", 24, 24, E_GADCON_ORIENT_TOP, rg);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_icon_add(evas, NULL, "enlightenment/shelf_position_bottom", 24, 24, E_GADCON_ORIENT_BOTTOM, rg);
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 1, 0, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_widget_table_object_append(o_table, o, 1, 0, 1, 1, 1, 1, 1, 1);

   /* _______ third column __________________________________________________*/
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, D_("Zoom"), 0);
   ob = e_widget_check_add(evas, D_("Zoom only one icon"), &(cfdata->zoom_one));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add (evas, D_("Factor:"));
   e_widget_framelist_object_append (of, ob);
   ob = e_widget_slider_add (evas, 1, 0, "%1.2f", 1.0, 3.0,
			     0.01, 0, &(cfdata->zoomfactor), NULL, 100);
   e_widget_on_change_hook_set(ob, _cb_slider_change, cfdata);
   e_widget_framelist_object_append (of, ob);
   ob = e_widget_label_add (evas, D_("Range:"));
   e_widget_framelist_object_append (of, ob);
   ob = e_widget_slider_add (evas, 1, 0, "%1.2f", 1.0, 4.0,
			     0.01, 0, &(cfdata->zoom_range), NULL, 100);
   e_widget_on_change_hook_set(ob, _cb_slider_change, cfdata);
   e_widget_framelist_object_append (of, ob);
   ob = e_widget_label_add (evas, D_("Duration:"));
   e_widget_framelist_object_append (of, ob);
   ob = e_widget_slider_add (evas, 1, 0, "%1.2f", 0.1, 0.5,
			     0.01, 0, &(cfdata->zoom_duration), NULL, 100);
   e_widget_on_change_hook_set(ob, _cb_slider_change, cfdata);
   e_widget_framelist_object_append (of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);


   of = e_widget_framelist_add(evas, D_("Auto Hide"), 0);
   /*ob = e_widget_check_add(evas, D_("Autohide"), &(cfdata->autohide));
     e_widget_framelist_object_append(of, ob);
     ob = e_widget_check_add(evas, D_("Hide Below Windows"), &(cfdata->hide_below_windows));
     e_widget_framelist_object_append(of, ob);
   */
   cfdata->hide_mode = 0;
   if (cfdata->autohide)
     cfdata->hide_mode = 1;
   else if (cfdata->hide_below_windows)
     cfdata->hide_mode = 2;

   rg = e_widget_radio_group_new(&cfdata->hide_mode);
   ob = e_widget_radio_add(evas, "None", 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, "Autohide", 1, rg);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, D_("Show Bar when Urgent"), &(cfdata->autohide_show_urgent));
   e_widget_framelist_object_append(of, ob);

   /*  ob = e_widget_radio_add(evas, "Hide Below Windows", 2, rg);
       e_widget_framelist_object_append(of, ob);
       //  e_widget_on_change_hook_set(ob, _cb_check_if_launcher_source, cfdata);
       */
   ob = e_widget_label_add (evas, D_("Hide Timeout:"));
   e_widget_framelist_object_append (of, ob);
   ob = e_widget_slider_add (evas, 1, 0, "%1.2f", 0.1, 1.0,
			     0.01, 0, &(cfdata->hide_timeout), NULL, 100);
   e_widget_on_change_hook_set(ob, _cb_slider_change, cfdata);
   e_widget_framelist_object_append (of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);


   of = e_widget_framelist_add(evas, D_("Other"), 0);
   ob = e_widget_check_add(evas, D_("Lock Deskflip on Edge"), &(cfdata->lock_deskswitch));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, D_("Ecomorph Features"), &(cfdata->ecomorph_features));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_widget_table_object_append(o_table, o, 2, 0, 1, 1, 1, 1, 1, 1);

   e_widget_list_object_append (o_all, o_table, 1, 1, 0.5);

   return o_all;
}


static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;
   Ng *ng;
   Ngi_Box *box;
   int restart = 0;

   ci = (Config_Item*) cfd->data;

   ng = ci->ng;

   if (ci->stacking != cfdata->stacking)
     {
	restart = 1;
	goto end;
     }
   else if (ci->autohide != cfdata->hide_mode)
     {
	restart = 1;
	goto end;
     }

   ng->hide_step = 0;
   //  evas_object_show(ng->clip);
   ngi_bar_show(ng);

   if (ci->show_label != cfdata->show_label)
     {
	ci->show_label = cfdata->show_label;

	if (ci->show_label)
	  evas_object_show(ng->label);
	else
	  evas_object_hide(ng->label);
     }

   ci->show_background = cfdata->show_background;

   if (ci->show_background)
     {
	evas_object_show(ng->bg_clip);
     }
   else
     {
	evas_object_hide(ng->bg_clip);
     }

   ci->size = (int) cfdata->size;
   ci->zoomfactor = cfdata->zoomfactor;
   ci->hide_timeout = cfdata->hide_timeout;
   ci->zoom_duration = cfdata->zoom_duration;
   ci->zoom_range = cfdata->zoom_range;
   ci->alpha = cfdata->alpha;
   ci->zoom_one = cfdata->zoom_one;
   ci->mouse_over_anim = cfdata->mouse_over_anim;
   ci->autohide = cfdata->hide_mode;
   ci->lock_deskswitch = cfdata->lock_deskswitch;
   ci->ecomorph_features = cfdata->ecomorph_features;

   if (ci->orient != cfdata->orient)
     {
	Eina_List *l;

	ci->orient = cfdata->orient;

	ngi_win_position_calc(ng->win);
	ngi_reposition(ng);
	ngi_input_extents_calc(ng, 1);

	evas_object_resize(ng->o_event, ng->win->w, ng->win->h);

	evas_object_move(ng->o_event, 0, 0);

	EINA_LIST_FOREACH (ng->boxes, l, box)
	  if (ng->horizontal)
	    edje_object_signal_emit(box->separator, "e,state,horizontal", "e");
	  else
	    edje_object_signal_emit(box->separator, "e,state,vertical", "e");

	switch(ci->orient)
	  {
	   case E_GADCON_ORIENT_LEFT:
	      edje_object_signal_emit(ng->o_bg, "e,state,bg_left", "e");
	      edje_object_signal_emit(ng->o_frame, "e,state,bg_left", "e");
	      break;
	   case E_GADCON_ORIENT_RIGHT:
	      edje_object_signal_emit(ng->o_bg, "e,state,bg_right", "e");
	      edje_object_signal_emit(ng->o_frame, "e,state,bg_right", "e");
	      break;
	   case E_GADCON_ORIENT_TOP:
	      edje_object_signal_emit(ng->o_bg, "e,state,bg_top", "e");
	      edje_object_signal_emit(ng->o_frame, "e,state,bg_top", "e");
	      break;
	   case E_GADCON_ORIENT_BOTTOM:
	      edje_object_signal_emit(ng->o_bg, "e,state,bg_bottom", "e");
	      edje_object_signal_emit(ng->o_frame, "e,state,bg_bottom", "e");
	  }
	ngi_thaw(ng);
     }

   e_config_domain_save("module.ng", ngi_conf_edd, ngi_config);

 end:

   if (restart)
     {
	ngi_free(ng);
	ci->autohide = cfdata->hide_mode;
	ci->stacking = cfdata->stacking;
	ci->size = (int) cfdata->size;
	ci->zoomfactor = cfdata->zoomfactor;
	ci->hide_timeout = cfdata->hide_timeout;
	ci->zoom_duration = cfdata->zoom_duration;
	ci->zoom_range = cfdata->zoom_range;
	ci->alpha = cfdata->alpha;
	ci->show_label = cfdata->show_label;
	ci->show_background = cfdata->show_background;
	ci->orient = cfdata->orient;
	ci->zoom_one = cfdata->zoom_one;
	ci->mouse_over_anim = cfdata->mouse_over_anim;
	ci->lock_deskswitch = cfdata->lock_deskswitch;
	ci->ecomorph_features = cfdata->ecomorph_features;
	ci->config_dialog = cfdata->cfd;

	e_config_domain_save("module.ng", ngi_conf_edd, ngi_config);

	ngi_new(ci);
	return 1;
     }

   if (!ci->autohide)
     {
	ng->hide_step = 0;
	ngi_win_position_calc(ng->win);
	ngi_thaw(ng);
     }

   return 1;
}

static void
_update_boxes(Ng *ng)
{
   Eina_List *l;
   Ngi_Box *box;
   Config_Box *cfg_box;

   while(ng->boxes)
     {
	box = ng->boxes->data;
	if (box->cfg->type == taskbar)
	  ngi_taskbar_remove(box);
	else if (box->cfg->type == launcher)
	  ngi_launcher_remove(box);
	else if (box->cfg->type == gadcon)
	  ngi_gadcon_remove(box);
     }

   ngi_freeze(ng);

   EINA_LIST_FOREACH (ng->cfg->boxes, l, cfg_box)
     {
	switch (cfg_box->type)
	  {
	   case launcher:
	      ngi_launcher_new(ng, cfg_box);
	      break;

	   case taskbar:
	      ngi_taskbar_new(ng, cfg_box);
	      break;

	   case gadcon:
	      ngi_gadcon_new(ng, cfg_box);
	      break;
	  }
     }

   ngi_thaw(ng);
}

/***************************************************************************************/




static void
_cb_box_add_taskbar(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   Config_Box *cfg_box;

   cfg_box = E_NEW(Config_Box, 1);
   cfg_box->type = taskbar;
   cfg_box->taskbar_adv_bordermenu = 0;
   cfg_box->taskbar_skip_dialogs = 0;
   cfg_box->taskbar_skip_dialogs = 1;
   cfg_box->taskbar_show_iconified = 1;
   cfg_box->taskbar_show_desktop = 0;

   cfdata->cfg->boxes = eina_list_append(cfdata->cfg->boxes, cfg_box);

   _update_boxes(cfdata->cfg->ng);

   _load_box_tlist(cfdata);
}

static void
_cb_box_add_launcher(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   Config_Box *cfg_box;

   cfg_box = E_NEW(Config_Box, 1);
   cfg_box->type = launcher;
   cfg_box->launcher_app_dir = eina_stringshare_add("default");
   cfdata->cfg->boxes = eina_list_append(cfdata->cfg->boxes, cfg_box);

   _update_boxes(cfdata->cfg->ng);

   _load_box_tlist(cfdata);
}

static void
_cb_box_add_gadcon(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   Config_Box *cfg_box;

   cfg_box = E_NEW(Config_Box, 1);
   cfg_box->type = gadcon;
   //cfg_box->gadcon_items = eina_stringshare_add("ng_gadcon-tmp");
   cfdata->cfg->boxes = eina_list_append(cfdata->cfg->boxes, cfg_box);

   _update_boxes(cfdata->cfg->ng);

   _load_box_tlist(cfdata);
}

static void
_cb_box_del(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   int selected = e_widget_ilist_selected_get(cfdata->ilist);

   if (selected < 0) return;

   Eina_List *boxes = cfdata->cfg->boxes;

   Config_Box *cfg_box  = eina_list_nth(boxes, selected);

   if (!cfg_box) return;

   cfdata->cfg->boxes = eina_list_remove(boxes, cfg_box);

   switch(cfg_box->type)
     {
      case launcher:
	 ngi_launcher_remove(cfg_box->box);
	 break;
      case taskbar:
	 ngi_taskbar_remove(cfg_box->box);
	 break;
      case gadcon:
	 ngi_gadcon_remove(cfg_box->box);
     }

   ngi_thaw(cfdata->cfg->ng);

   _load_box_tlist(cfdata);
}

static Evas_Object *
_basic_create_box_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ol, *of, *ob, *ot;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);
   if (cfdata->cfg_box->type == launcher)
     {
	cfdata->app_dir = strdup(cfdata->cfg_box->launcher_app_dir);

	of = e_widget_frametable_add(evas, D_("Launcher Settings"), 0);
	ol = e_widget_ilist_add(evas, 0, 0, (const char**)&(cfdata->app_dir));
	cfdata->tlist_box = ol;
	_load_ilist(cfdata);
	e_widget_size_min_set(ol, 140, 140);
	e_widget_frametable_object_append(of, ol, 0, 0, 1, 2, 1, 1, 1, 1);

	ot = e_widget_table_add(evas, 0);
	ob = e_widget_button_add(evas, D_("Add"), "widget/add", _cb_add, cfdata, NULL);
	e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 1, 0);
	ob = e_widget_button_add(evas, D_("Delete"), "widget/del", _cb_del, cfdata, NULL);
	e_widget_table_object_append(ot, ob, 0, 1, 1, 1, 1, 1, 1, 0);
	ob = e_widget_button_add(evas, D_("Configure"), "widget/config", _cb_config, cfdata, NULL);
	e_widget_table_object_append(ot, ob, 0, 2, 1, 1, 1, 1, 1, 0);

	e_widget_frametable_object_append(of, ot, 2, 0, 1, 1, 1, 1, 1, 0);
	e_widget_list_object_append(o, of, 0, 1, 0.5);
     }
   else if (cfdata->cfg_box->type == taskbar)
     {
	of = e_widget_framelist_add(evas, D_("Taskbar Settings"), 0);

	ob = e_widget_check_add(evas, D_("Dont Show Dialogs"), &(cfdata->cfg_box->taskbar_skip_dialogs));
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_check_add(evas, D_("Advanced Window Menu"), &(cfdata->cfg_box->taskbar_adv_bordermenu));
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_check_add(evas, D_("Only Show Current Desk"), &(cfdata->cfg_box->taskbar_show_desktop));
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_check_add(evas, D_("Group Applications by Window Class"), &(cfdata->cfg_box->taskbar_group_apps));
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_check_add(evas, D_("Append new Applications on the right Side"), &(cfdata->cfg_box->taskbar_append_right));
	e_widget_framelist_object_append(of, ob);

	e_widget_list_object_append(o, of, 1, 1, 0.5);

	of = e_widget_framelist_add(evas, D_("Iconified Applications"), 0);
	rg = e_widget_radio_group_new(&cfdata->cfg_box->taskbar_show_iconified);
	ob = e_widget_radio_add(evas, "Not Shown", 0, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, "Show", 1, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, "Only Iconified", 2, rg);
	e_widget_framelist_object_append(of, ob);

	e_widget_list_object_append(o, of, 1, 1, 0.5);
     }

   return o;
}

static int
_basic_apply_box_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->cfg_box->type == launcher)
     {
	cfdata->cfg_box->launcher_app_dir = strdup(cfdata->app_dir);
	ngi_launcher_remove(cfdata->cfg_box->box);
	//      ngi_launcher_new(cfdata->cfg->ng, cfdata->cfg_box);
	_update_boxes(cfdata->cfg->ng);
     }
   else if (cfdata->cfg_box->type == taskbar)
     {
	ngi_taskbar_empty(cfdata->cfg_box->box);
	ngi_taskbar_fill(cfdata->cfg_box->box);
     }
   return 1;

}

/* urgh */
static void *
_create_box_data(E_Config_Dialog *cfd)
{
   return cfd->data;
}

static void
_free_box_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* free app_dir ?*/
}

static void
_cb_box_config(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;

   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];

   int selected = e_widget_ilist_selected_get(cfdata->ilist);

   if (selected < 0) return;

   Eina_List *boxes = cfdata->cfg->boxes;

   cfdata->cfg_box= eina_list_nth(boxes, selected);

   if (!cfdata->cfg_box) return;

   if (cfdata->cfg_box->type == gadcon)
     {
	ngi_gadcon_config(cfdata->cfg_box->box);
     }
   else
     {
	v = E_NEW(E_Config_Dialog_View, 1);

	v->create_cfdata = _create_box_data;
	v->free_cfdata = _free_box_data;
	v->basic.apply_cfdata = _basic_apply_box_data;
	v->basic.create_widgets = _basic_create_box_widgets;
	v->advanced.apply_cfdata = NULL;
	v->advanced.create_widgets = NULL;

	snprintf(buf, sizeof(buf), "%s/e-module-ng.edj", e_module_dir_get(ngi_config->module));
	cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
				  D_("Itask NG Bar Configuration"),
				  "e", "_e_mod_ngi_config_dialog_add_box", buf, 0, v, cfdata);
     }
}

static void *
_create_data2(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;

   Ngi_Box *box = cfd->data;

   ci = (Config_Item*) box->ng->cfg;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(ci, cfdata);
   cfdata->cfg_box = box->cfg;

   return cfdata;
}

void
ngi_configure_box(Ngi_Box *box)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];

   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data2;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_box_data;
   v->basic.create_widgets = _basic_create_box_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;

   snprintf(buf, sizeof(buf), "%s/e-module-ng.edj", e_module_dir_get(ngi_config->module));
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
			     D_("Itask NG Bar Configuration"),
			     "e", "_e_mod_ngi_config_dialog_add_box", buf, 0, v, box);
}

static void
_cb_box_up(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   int selected = e_widget_ilist_selected_get(cfdata->ilist);

   if (selected < 0) return;

   Eina_List *boxes = cfdata->cfg->boxes;

   Eina_List *l = eina_list_nth_list(boxes, selected);

   if (!l || !l->prev) return;

   Config_Box *cfg_box = l->data;

   boxes = eina_list_remove(boxes, cfg_box);
   cfdata->cfg->boxes = eina_list_prepend_relative(boxes, cfg_box, l->prev->data);

   _update_boxes(cfdata->cfg->ng);

   _load_box_tlist(cfdata);

   e_widget_ilist_selected_set(cfdata->ilist, selected - 1);
}

static void
_cb_box_down(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   int selected = e_widget_ilist_selected_get(cfdata->ilist);

   if (selected < 0) return;

   Eina_List *boxes = cfdata->cfg->boxes;

   Eina_List *l = eina_list_nth_list(boxes, selected);

   if (!l || !l->next) return;

   Config_Box *cfg_box = l->data;

   boxes = eina_list_remove(boxes, cfg_box);
   cfdata->cfg->boxes = eina_list_append_relative(boxes, cfg_box, l->next->data);

   _update_boxes(cfdata->cfg->ng);

   _load_box_tlist(cfdata);

   e_widget_ilist_selected_set(cfdata->ilist, selected + 1);
}

static void
_load_box_tlist(E_Config_Dialog_Data *cfdata)
{
   char buf[4096];

   e_widget_ilist_clear(cfdata->ilist);

   int cnt = 1;
   Eina_List *l;
   Config_Box *cfg_box;
   char *blub;

   for(l = cfdata->cfg->boxes; l; l = l->next)
     {
	cfg_box = l->data;
	switch (cfg_box->type)
	  {
	   case launcher:
	      snprintf(buf, sizeof(buf), "%i Launcher", cnt);
	      blub = strdup(buf);

	      e_widget_ilist_append(cfdata->ilist, NULL, blub, NULL, cfg_box, blub);
	      break;

	   case taskbar:
	      snprintf(buf, sizeof(buf), "%i Taskbar", cnt);
	      blub = strdup(buf);
	      e_widget_ilist_append(cfdata->ilist, NULL, blub, NULL, cfg_box, blub);
	      break;

	   case gadcon:
	      snprintf(buf, sizeof(buf), "%i Gadcon", cnt);
	      blub = strdup(buf);
	      e_widget_ilist_append(cfdata->ilist, NULL, blub, NULL, cfg_box, blub);
	      break;
	  }
	cnt++;
     }
   e_widget_ilist_go(cfdata->ilist);
}

static void
_cb_add(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   e_entry_dialog_show(D_("Create new Itask NG source"), "enlightenment/e",
		       D_("Enter a name for this new Application Launcher:"), "", NULL, NULL,
		       _cb_entry_ok, NULL, cfdata);
}

static void
_cb_del(void *data, void *data2)
{
   char buf[4096];
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   snprintf(buf, sizeof(buf), D_("You requested to delete \"%s\".<br><br>"
				 "Are you sure you want to delete this ng source?"),
	    cfdata->app_dir);

   e_confirm_dialog_show(D_("Are you sure you want to delete this Itask NG source?"),
			 "enlightenment/exit", buf, NULL, NULL,
			 _cb_confirm_dialog_yes, NULL, cfdata, NULL, NULL, NULL);
}

static void
_cb_config(void *data, void *data2)
{
   char path[4096];
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   snprintf(path, sizeof(path), "%s/.e/e/applications/bar/%s/.order",
	    e_user_homedir_get(), cfdata->app_dir);

   e_configure_registry_call("internal/ibar_other",
			     e_container_current_get(e_manager_current_get()),
			     path);
}

static void
_cb_entry_ok(void *data, char *text)
{
   char buf[4096];
   char tmp[4096];
   FILE *f;

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s",
	    e_user_homedir_get(), text);

   if (!ecore_file_exists(buf))
     {
	ecore_file_mkdir(buf);

	snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s/.order",
		 e_user_homedir_get(), text);

	f = fopen(buf, "w");
	if (f)
	  {
	     // Populate this .order file with some defaults
	     snprintf(tmp, sizeof(tmp), "xterm.desktop\n"
		      "firefox.desktop\n"
		      "gimp.desktop\n" "xmms.desktop\n");
	     fwrite(tmp, sizeof(char), strlen(tmp), f);
	     fclose(f);
	  }
     }

   _load_ilist((E_Config_Dialog_Data*) data);
}

static void
_cb_confirm_dialog_yes(void *data)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", e_user_homedir_get(), cfdata->app_dir);

   if (ecore_file_is_dir(buf))
     ecore_file_recursive_rm(buf);

   _load_ilist(cfdata);
}

static void
_load_ilist(E_Config_Dialog_Data *cfdata)
{
   Eina_List *dirs, *l;
   char buf[4096], *file;
   int selnum = -1;
   const char *home;
   int i = 0;
   e_widget_ilist_clear(cfdata->tlist_box);

   home = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar", home);
   dirs = ecore_file_ls(buf);

   EINA_LIST_FOREACH(dirs, l, file)
     {
	if (file[0] == '.') continue;

	snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", home, file);
	if (ecore_file_is_dir(buf))
	  {
	     e_widget_ilist_append(cfdata->tlist_box, NULL, file, NULL, NULL, file);
	     if ((cfdata->app_dir) && (!strcmp(cfdata->app_dir, file)))
	       selnum = i;
	     i++;
	  }
     }

   e_widget_ilist_go(cfdata->tlist_box);
   if (selnum >= 0)
     e_widget_ilist_selected_set(cfdata->tlist_box, selnum);
}

static void
_cb_slider_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data*) data;
   Ng *ng = cfdata->cfg->ng;

   ng->cfg->size = (int) cfdata->size;
   ng->size = ng->cfg->size;

   ng->cfg->zoomfactor = cfdata->zoomfactor;
   ng->cfg->zoom_duration = cfdata->zoom_duration;
   ng->cfg->zoom_range = cfdata->zoom_range;
   ng->cfg->hide_timeout = cfdata->hide_timeout;
   ng->cfg->alpha = cfdata->alpha;

   ng->hide_step = 0;

   evas_object_color_set(ng->bg_clip, ng->cfg->alpha, ng->cfg->alpha, ng->cfg->alpha, ng->cfg->alpha);
   ngi_thaw(ng);
}
