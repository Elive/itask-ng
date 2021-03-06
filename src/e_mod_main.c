#include "e_mod_main.h"
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>

static int _ngi_win_free(Ngi_Win *win);
static Ngi_Win *_ngi_win_new(Ng *ng);

static Eina_Bool _ngi_cb_container_resize(void *data, int type, void *event);
/* static int _ngi_cb_desk_change(void *data, int type, void *event); */

static void _ngi_win_cb_drop_enter(void *data, const char *type, void *event_info);
static void _ngi_win_cb_drop_move(void *data, const char *type, void *event_info);
static void _ngi_win_cb_drop_end(void *data, const char *type, void *event_info);
static void _ngi_win_cb_drop_leave(void *data, const char *type, void *event_info);

static Eina_Bool _ngi_win_cb_mouse_in(void *data, int type, void *event);
static Eina_Bool _ngi_win_cb_mouse_out(void *data, int type, void *event);
static Eina_Bool _ngi_win_cb_mouse_down(void *data, int type, void *event);
static Eina_Bool _ngi_win_cb_mouse_up(void *data, int type, void *event);
static Eina_Bool _ngi_win_cb_mouse_move(void *data, int type, void *event);
static Eina_Bool _ngi_win_cb_mouse_wheel(void *data, int type, void *event);

static void _ngi_zoom_in(Ng *ng);
static void _ngi_zoom_out(Ng *ng);
static void _ngi_item_appear(Ng *ng, Ngi_Item *it);
static void _ngi_item_disappear(Ng *ng, Ngi_Item *it);
static void _ngi_zoom_function(Ng *ng, double d, double *zoom, double *disp);
static int _ngi_zoom_function2(Ng *ng);
static Eina_Bool _ngi_animator(void *data);
static void _ngi_redraw(Ng *ng);
static int _ngi_autohide(Ng *ng, int hide);

/* static int _ngi_mouse_out_timer_cb(void *data); */
static void _ngi_item_activate(Ng *ng);
static Ngi_Item *_ngi_item_at_position_get(Ng *ng);


void ngi_hide_clip(Ng *ng);

static int initialized = 0;

E_Config_DD *ngi_conf_edd = NULL;
E_Config_DD *ngi_conf_item_edd = NULL;
E_Config_DD *ngi_conf_box_edd = NULL;
E_Config_DD *ngi_conf_gadcon_edd = NULL;

Config      *ngi_config = NULL;

E_Int_Menu_Augmentation *maug = NULL;

#define TEXT_DIST 15
#define SEPARATOR_WIDTH 1

Ng *
ngi_new(Config_Item *cfg)
{
   Ng *ng;
   E_Zone *zone;
   Config_Box * cfg_box;
   Eina_List *l;
  
   zone = (E_Zone*) e_util_container_zone_number_get(cfg->container, cfg->zone);
  
   if (!zone) return NULL;

   ng = E_NEW(Ng, 1);
   cfg->ng = ng;
   ng->cfg = cfg;
   ng->cfg->mouse_over_anim = 0;
  
   ng->zone  = zone;

   ng->win = _ngi_win_new(ng);

   ng->o_bg = NULL;
   ng->o_frame = NULL;
   ng->animator = NULL;
   ng->items_scaling = NULL;
   ng->item_active = NULL;
   ng->item_drag = NULL;
   ng->zoom = 1.0;
   ng->item_spacing = 2;
   ng->state = unzoomed;
   ng->hide_step = 0;
   ng->hide_state = show;
   ng->mouse_in = 0;
   ng->zoom_out = 1;
   ng->dnd = 0;
  
   ng->mouse_out_timer = NULL;
   ng->mouse_in_timer = NULL;
   ng->menu_wait_timer = NULL;
   ng->effect_timer = NULL;
  
   ng->start = 0;

   ng->w = 0;
   ng->h = 0;
   ng->pos = 0;

   ng->separator_width = 0;
  
   ng->size = ng->cfg->size;
  
   ng->clip = evas_object_rectangle_add(ng->win->evas);
   evas_object_move(ng->clip, -100004, -100004);
   evas_object_resize(ng->clip, 200008, 200008);
   evas_object_color_set(ng->clip, 255, 255, 255, 255);

   ng->bg_clip = evas_object_rectangle_add(ng->win->evas);
   evas_object_move(ng->bg_clip, -100004, -100004);
   evas_object_resize(ng->bg_clip, 200008, 200008);
   evas_object_color_set(ng->bg_clip, cfg->alpha, cfg->alpha, 
			 cfg->alpha, cfg->alpha);

   evas_object_show(ng->bg_clip);

   ng->o_bg = edje_object_add(ng->win->evas);
   if (!e_theme_edje_object_set(ng->o_bg, 
				"base/theme/modules/itask-ng", 
				"e/modules/itask-ng/bg"))

     edje_object_file_set(ng->o_bg, ngi_config->theme_path, 
			  "e/modules/itask-ng/bg");

   evas_object_show(ng->o_bg);
   evas_object_clip_set(ng->o_bg, ng->bg_clip);

   ng->o_frame = edje_object_add(ng->win->evas);
   if (!e_theme_edje_object_set(ng->o_frame, 
				"base/theme/modules/itask-ng", 
				"e/modules/itask-ng/bg_frame"))

     edje_object_file_set(ng->o_frame, ngi_config->theme_path, 
			  "e/modules/itask-ng/bg_frame");

   evas_object_show(ng->o_frame);

   ng->label = edje_object_add(ng->win->evas);
  
   if (ng->horizontal)
     {
	if (!e_theme_edje_object_set(ng->label, 
				     "base/theme/modules/itask-ng", 
				     "e/modules/itask-ng/label"))

	  edje_object_file_set(ng->label, ngi_config->theme_path, 
			       "e/modules/itask-ng/label");

	ng->pos = ng->win->w/2;
     }
   else
     {
	ng->pos = ng->win->h/2;
     }

   if (cfg->show_label)
     evas_object_show(ng->label);

   if (cfg->show_background) evas_object_show(ng->bg_clip);

   switch(cfg->orient)
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
  
   ng->boxes = NULL;
   ngi_freeze(ng);
  
   EINA_LIST_FOREACH (cfg->boxes, l, cfg_box)
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

   if (cfg->autohide)
     {
	ng->hide_step = ng->size + 15;
	ng->hide_state = hidden;
     }
  
   ng->handlers = eina_list_append
     (ng->handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_IN, _ngi_win_cb_mouse_in, ng));
   ng->handlers = eina_list_append
     (ng->handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_OUT, _ngi_win_cb_mouse_out, ng));
   ng->handlers = eina_list_append
     (ng->handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_DOWN, _ngi_win_cb_mouse_down, ng));
   ng->handlers = eina_list_append
     (ng->handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_UP, _ngi_win_cb_mouse_up, ng));
   ng->handlers = eina_list_append
     (ng->handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_WHEEL, _ngi_win_cb_mouse_wheel, ng));
   ng->handlers = eina_list_append
     (ng->handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_MOVE, _ngi_win_cb_mouse_move, ng));

   ng->show_bar = 0;
   ngi_thaw(ng);
   ngi_win_show(ng->win);

   ngi_config->instances = eina_list_append(ngi_config->instances, ng);

   return ng;
}

void 
ngi_free(Ng *ng)
{
   Ngi_Box *box;
   Ecore_Event_Handler *h;

   ngi_config->instances = eina_list_remove(ngi_config->instances, ng);
  
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

   if (ng->animator)        ecore_timer_del(ng->animator);
   if (ng->menu_wait_timer) ecore_timer_del(ng->menu_wait_timer);
   if (ng->mouse_out_timer) ecore_timer_del(ng->mouse_out_timer);
   if (ng->mouse_in_timer)  ecore_timer_del(ng->mouse_in_timer);
   if (ng->effect_timer)    ecore_timer_del(ng->effect_timer);
  
   if (ng->o_bg)    evas_object_del(ng->o_bg);
   if (ng->o_frame) evas_object_del(ng->o_frame);
   if (ng->clip)    evas_object_del(ng->clip);
   if (ng->bg_clip) evas_object_del(ng->bg_clip);

   if (ng->win) _ngi_win_free(ng->win);

   EINA_LIST_FREE(ng->handlers, h)
     ecore_event_handler_del(h);
  
   free(ng);
}


/***************************************************************************/

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "itask-ng" };

static int 
_ngi_init_timer_cb(void *data)
{
   Eina_List *l;
   Config_Item *ci;
  
   EINA_LIST_FOREACH (ngi_config->items, l, ci)
     ngi_new(ci);

   initialized = 1;
  
   return 0;

}

/* menu item callback(s) */
static void 
_e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   ngi_instances_config(e_util_zone_current_get(e_manager_current_get())->container, "");
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, D_("Itask-NG"));
   //e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf");
   e_menu_item_callback_set(mi, _e_mod_run_cb, NULL);
}


EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4096];
   ngi_config = NULL;

   /* Location of message catalogs for localization */
   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");
   
   ngi_conf_gadcon_edd = E_CONFIG_DD_NEW("Ngi_Config_Gadcon", Config_Gadcon);
   E_CONFIG_VAL(ngi_conf_gadcon_edd, Config_Gadcon, name, STR);

   ngi_conf_box_edd = E_CONFIG_DD_NEW("Ngi_Config_Item_Box", Config_Box);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, type, INT);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, launcher_app_dir, STR);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, launcher_lock_dnd, INT);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, taskbar_skip_dialogs, INT);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, taskbar_adv_bordermenu, INT);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, taskbar_show_iconified, INT);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, taskbar_show_desktop, INT);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, taskbar_append_right, INT);
   E_CONFIG_VAL(ngi_conf_box_edd, Config_Box, taskbar_group_apps, INT);
   E_CONFIG_LIST(ngi_conf_box_edd, Config_Box, gadcon_items, ngi_conf_gadcon_edd);
    
   ngi_conf_item_edd = E_CONFIG_DD_NEW("Ngi_Config_Item", Config_Item);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, show_label, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, show_background, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, container, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, zone, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, orient, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, size, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, autohide, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, autohide_show_urgent, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, hide_timeout, FLOAT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, zoom_duration, FLOAT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, zoomfactor, FLOAT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, zoom_range, FLOAT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, zoom_one, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, hide_below_windows, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, alpha, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, sia_remove, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, stacking, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, mouse_over_anim, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, lock_deskswitch, INT);
   E_CONFIG_VAL(ngi_conf_item_edd, Config_Item, ecomorph_features, INT);
   E_CONFIG_LIST(ngi_conf_item_edd, Config_Item, boxes, ngi_conf_box_edd);

     ngi_conf_edd = E_CONFIG_DD_NEW("Ngi_Config", Config);
   E_CONFIG_LIST(ngi_conf_edd, Config, items, ngi_conf_item_edd);

   ngi_config = (Config*) e_config_domain_load("module.itask-ng", ngi_conf_edd);

   if (!ngi_config) ngi_config = E_NEW(Config, 1);

   ngi_config->cfd = NULL;
   ngi_config->module = m;

   snprintf(buf, sizeof(buf), "%s/itask-ng.edj", e_module_dir_get(m));
   ngi_config->theme_path = strdup(buf);

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/", e_user_homedir_get());
   if (!ecore_file_mkdir(buf) && !ecore_file_is_dir(buf))
     {
	e_error_message_show("Error creating directory:\n %s\n",buf);
	return m;
     }

   /* HACK - as long as we dont have an e_initialisation_finished_cb ... */
   /* ecore_timer_add(2.0,  */
  
   e_configure_registry_item_add("extensions/itask_ng", 40,
				 D_("Itask NG"), NULL,
				 "enlightenment/shelf",
				 ngi_instances_config);

   ngi_config->handlers = eina_list_append
     (ngi_config->handlers, ecore_event_handler_add
      (E_EVENT_CONTAINER_RESIZE, _ngi_cb_container_resize, NULL));

   ngi_taskbar_init();
   ngi_gadcon_init();
  
   if (ecore_x_screen_is_composited(0) || e_config->use_composite)
     ngi_config->use_composite = 1;
   else
     ngi_config->use_composite = 0;

   e_module_delayed_set(m, 1);
   _ngi_init_timer_cb(NULL);
   
   maug = e_int_menus_menu_augmentation_add
     ("config/1", _e_mod_menu_add, NULL, NULL, NULL);

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m)
{
   Ecore_Event_Handler *h;
   Ng *ng;
   Eina_List *l, *ll;
  
   if (maug)
     {
	e_int_menus_menu_augmentation_del("config/1", maug);
	maug = NULL;
     }
  
   ngi_gadcon_shutdown();
  
   EINA_LIST_FOREACH_SAFE(ngi_config->instances, l, ll, ng)
     {
	if (ng->cfg->config_dialog)
	  e_object_del(E_OBJECT(ng->cfg->config_dialog));
	ngi_free(ng);
     }

   EINA_LIST_FREE(ngi_config->handlers, h)
     ecore_event_handler_del(h);

   while (ngi_config->items)
     {
	Config_Item *ci;
	Config_Box *cfg_box;
	Eina_List *l, *ll;
	Config_Gadcon *cg;

	ci = (Config_Item*) ngi_config->items->data;
	ngi_config->items = eina_list_remove_list(ngi_config->items, 
						  ngi_config->items);

	EINA_LIST_FOREACH (ci->boxes, l, cfg_box)
	  {
	     if (cfg_box->type == launcher && cfg_box->launcher_app_dir)
	       eina_stringshare_del(cfg_box->launcher_app_dir);

	     if (cfg_box->type == gadcon)
	       {
		  EINA_LIST_FOREACH (cfg_box->gadcon_items, ll, cg)
		    eina_stringshare_del(cg->name);
	       }
	  }
	free(ci);
     }

   free(ngi_config->theme_path);
   free(ngi_config);
   ngi_config = NULL;

   E_CONFIG_DD_FREE(ngi_conf_gadcon_edd);
   E_CONFIG_DD_FREE(ngi_conf_box_edd);
   E_CONFIG_DD_FREE(ngi_conf_item_edd);
   E_CONFIG_DD_FREE(ngi_conf_edd);
  
   e_configure_registry_item_del("extensions/itask_ng");

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.itask-ng", ngi_conf_edd, ngi_config);
   return 1;
}

static Ngi_Win *
_ngi_win_new(Ng *ng)
{
   Ngi_Win *win;
   const char *drop[] = 
     {
       "text/uri-list",
       "text/x-moz-url", 
       "enlightenment/x-file", 
       "enlightenment/desktop", 
       "enlightenment/border"
     };  

   win = E_NEW(Ngi_Win, 1);
   if (!win) return NULL;

   win->ng = ng;
   win->edge = 0;
   win->fake_ewin = NULL;
   win->drop_handler = NULL;
   win->ee = NULL;
   win->evas_win = 0;
  
   if (ng->cfg->stacking != on_desk)
     {
	win->ee = e_canvas_new(ng->zone->container->win, 
			       0, 0, 0, 0, 1, 1, &(win->evas_win)); 

	if (ngi_config->use_composite)
	  {
	     ecore_evas_alpha_set(win->ee, 1);
	     win->evas_win = ecore_evas_software_x11_window_get(win->ee);

	     XRectangle rect;
	     rect.x = 0;
	     rect.y = 0;
	     rect.width = 0;
	     rect.height = 0;
	     XShapeCombineRectangles
	       ((Display*)ecore_x_display_get(),
		win->evas_win, ShapeInput, 0, 0,
		&rect, 1, ShapeSet, Unsorted);
	     ecore_x_netwm_window_type_set
	       (win->evas_win, ECORE_X_WINDOW_TYPE_DOCK); 
	  }
	else
	  {
	     ecore_evas_shaped_set(win->ee, 1);
	  }

	ecore_evas_avoid_damage_set(win->ee, 1);
   
	e_canvas_add(win->ee);

	win->evas = ecore_evas_get(win->ee);
     }
   else
     {
	win->evas = win->ng->zone->container->bg_evas;
     }

   win->input = ecore_x_window_input_new(ng->zone->container->win, 0, 0, 1, 1); 
   ecore_x_window_show(win->input);
   e_drop_xdnd_register_set(win->input, 1);

   win->fake_iwin = E_OBJECT_ALLOC(E_Win, E_WIN_TYPE, NULL);
   win->fake_iwin->x = 0;
   win->fake_iwin->y = 0;
   win->fake_iwin->w = 1;
   win->fake_iwin->h = 1;
   win->fake_iwin->evas_win = win->input;

   if (ng->cfg->stacking != above_all)
     {
	win->edge = ecore_x_window_input_new(ng->zone->container->win, 0, 0, 1, 1);
	ecore_x_window_show(win->edge);
	e_drop_xdnd_register_set(win->edge, 1);

	win->fake_ewin = E_OBJECT_ALLOC(E_Win, E_WIN_TYPE, NULL);
	win->fake_ewin->x = 0;
	win->fake_ewin->y = 0;
	win->fake_ewin->w = 1;
	win->fake_ewin->h = 1;
	win->fake_ewin->evas_win = win->edge;

	win->drop_handler = e_drop_handler_add
	  (E_OBJECT(win->fake_ewin), ng,
	   _ngi_win_cb_drop_enter, _ngi_win_cb_drop_move,
	   _ngi_win_cb_drop_leave, _ngi_win_cb_drop_end,
	   drop, 5, 0, 0, 0, 0);
     }
  
   switch (ng->cfg->stacking)
     {
      case below_fullscreen:
	 e_container_window_raise(ng->zone->container, win->evas_win, E_LAYER_ABOVE);
         if (ng->cfg->lock_deskswitch)
	   e_container_window_raise(ng->zone->container, win->edge, 999);
	 else
	   e_container_window_raise(ng->zone->container, win->edge, E_LAYER_ABOVE);
         
         e_container_window_raise(ng->zone->container, win->input, E_LAYER_ABOVE);
	 break;

      case on_desk:
	 if (ng->cfg->lock_deskswitch)
	   e_container_window_raise(ng->zone->container, win->edge, 999);
	 else
	   e_container_window_raise(ng->zone->container, win->edge, E_LAYER_BELOW);

	 e_container_window_raise(ng->zone->container, win->input, E_LAYER_BELOW);
	 break;  

      case above_all:
	 e_container_window_raise(ng->zone->container, win->evas_win, E_LAYER_FULLSCREEN);
	 e_container_window_raise(ng->zone->container, win->input, 999);
     }

   ngi_win_position_calc(win);

   return win;
}

static int 
_ngi_win_free(Ngi_Win *win)
{
   if (win->ee) 
     {
	e_canvas_del(win->ee);
	ecore_evas_free(win->ee);
     }
  
   e_drop_xdnd_register_set(win->input, 0);
   ecore_x_window_hide(win->input);
   ecore_x_window_free(win->input);
   free(win->fake_iwin);

   if (win->edge)
     {
	e_drop_handler_del(win->drop_handler);
 
	e_drop_xdnd_register_set(win->edge, 0);
	ecore_x_window_hide(win->edge);
	ecore_x_window_free(win->edge);
	free(win->fake_ewin);
     }
  
   free(win);

   return 1;
}

static Eina_Bool 
_ngi_cb_container_resize(void *data, int ev_type, void *event_info)
{
   Config_Item *ci;
   E_Zone *zone;
   Eina_List *l;
   Ng *ng;

   if (!initialized) return 1;
  
   EINA_LIST_FOREACH (ngi_config->items, l, ci)
     {
	if (!ci->ng)
	  {
	     ngi_new(ci);
	     continue;
	  }

	ng = ci->ng;
	zone = e_util_container_zone_number_get(ci->container, ci->zone);
	if (!zone)
	  {
	     ngi_free(ng);
	     continue;
	  }

	ngi_win_position_calc(ng->win);
	ngi_reposition(ng);
	ngi_input_extents_calc(ng, 1);
	ngi_thaw(ng);
     }
   return 1;
}

void 
ngi_win_show(Ngi_Win *win)
{
   if (win->ee) ecore_evas_show(win->ee);
}

void 
ngi_win_hide(Ngi_Win *win)
{
   if (win->ee) ecore_evas_hide(win->ee);
}

void 
ngi_win_position_calc(Ngi_Win *win)
{
   Ng *ng = win->ng;

   E_Gadcon_Orient orient = (E_Gadcon_Orient) ng->cfg->orient;
   int size = 250;

   switch (orient)
     {
      case E_GADCON_ORIENT_LEFT:
	 win->w = size;
	 win->h = ng->zone->h;
	 win->x = 0;
	 win->y = 0;
	 ng->horizontal = 0;
	 break;
      case E_GADCON_ORIENT_RIGHT:
	 win->w = size;
	 win->h = ng->zone->h;
	 win->x = ng->zone->w - size;
	 win->y = 0;
	 ng->horizontal = 0;
	 break;
      case E_GADCON_ORIENT_TOP:
	 win->w = ng->zone->w;
	 win->h = size;
	 win->x = 0;
	 win->y = 0;
	 ng->horizontal = 1;
	 break;
      case E_GADCON_ORIENT_BOTTOM:
	 win->w = ng->zone->w;
	 win->h = size;
	 win->x = 0;
	 win->y = ng->zone->h - win->h;
	 ng->horizontal = 1;
	 break;
      default:
	 break;
     }

   if (win->ee)
     {
	ecore_evas_move_resize
	  (win->ee, win->ng->zone->x + win->x, 
	   win->ng->zone->y + win->y, win->w, win->h);
     }
}

static Eina_Bool
_ngi_mouse_in_timer(void *data)
{
   Ng *ng = data;

   if (!ng->mouse_in) return 0;
  
   if (ng->cfg->autohide && ng->hide_state != show)  
     _ngi_autohide(ng, 0);

   if (ng->cfg->stacking != on_desk)
     {
	if (!ng->dnd)
	  { 
	     evas_event_feed_mouse_in(ng->win->evas, 0, NULL);
	     _ngi_item_activate(ng);  
	  }
     }
   
   ng->mouse_in_timer = NULL;

   ng->zoom_out = 0;
   ngi_animate(ng);  

   return 0;
}

void
ngi_mouse_in(Ng *ng)
{
   ng->mouse_in = 1;
   ng->zoom_out = 0;
   
   if (ng->mouse_out_timer)
     {
	ecore_timer_del(ng->mouse_out_timer);
	ng->mouse_out_timer = NULL;
     }
  
   if (ng->cfg->stacking != above_all)
     { 
	if (!ng->mouse_in_timer)
	  ng->mouse_in_timer = ecore_timer_add(0.1, _ngi_mouse_in_timer, ng);
     }
   else
     { 
	_ngi_mouse_in_timer(ng);
     }
}

static Eina_Bool 
_ngi_mouse_out_timer(void *data)
{
   Ng *ng = data;

   if (ng->mouse_in) return 0;
  
   ng->mouse_out_timer = NULL;

   if (ng->cfg->stacking != on_desk) /*XXX*/
     evas_event_feed_mouse_out(ng->win->evas, 0, NULL);
   ITEM_MOUSE_OUT(ng->item_active);
   edje_object_signal_emit(ng->label, "e,state,label_hide", "e");
  
   ngi_input_extents_calc(ng, 1);

   ngi_animate(ng);

   return 0;
}

static Eina_Bool 
_ngi_mouse_out_wait(void *data)
{
   Ng *ng = data;
   
   ng->zoom_out = 1;

   ng->mouse_out_timer = ecore_timer_add
     (ng->cfg->hide_timeout, _ngi_mouse_out_timer, ng);

   return 0;
}

void
ngi_mouse_out(Ng *ng)
{
   ng->mouse_in = 0;

   if (ng->mouse_out_timer)
     ecore_timer_del(ng->mouse_out_timer);
   
   ng->mouse_out_timer = ecore_timer_add
     (0.01, _ngi_mouse_out_wait, ng);
}

static Eina_Bool 
_ngi_win_cb_mouse_in(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_In *ev = event;
   Ng *ng = data;

   if (ev->win != ng->win->input && ev->win != ng->win->edge)
     return 1;

   ng->pos = ng->horizontal ? 
     (ev->root.x - ng->zone->x): 
     (ev->root.y - ng->zone->y);

   ngi_mouse_in(ng);

   evas_event_feed_mouse_in(ng->win->evas, 0, NULL); 
   evas_object_focus_set(ng->bg_clip, 1);
  
   return 1;
}

static Eina_Bool
_ngi_win_cb_mouse_out(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Out *ev = event;
   Ng *ng = data;

   if (ev->win != ng->win->input && ev->win != ng->win->edge)
     return 1;

   ngi_mouse_out(ng);

   if (ng->cfg->stacking != on_desk)
     evas_event_feed_mouse_out(ng->win->evas, 0, NULL);

   return 1;
}

static Eina_Bool
_ngi_menu_wait_after_timer_cb(void *data)
{
   Ng *ng = data;
  
   ng->menu_wait_timer = NULL;
   ngi_animate(ng);
  
   return 0;
}

static Eina_Bool
_ngi_menu_wait_timer_cb(void *data)
{
   Ng *ng = data;
  
   if (e_menu_grab_window_get())
     return 1;

   ng->menu_wait_timer =
     ecore_timer_add(1.0, _ngi_menu_wait_after_timer_cb, ng);
      
   return 0;
}

static Eina_Bool
_ngi_win_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Ng *ng = data;

   if ((ev->event_window != ng->win->input) &&
       (ev->event_window != ng->win->edge))
     return 1;
  
   Ngi_Item *it = ng->item_active;

   if (ev->buttons == 2)
     {
	ngi_configure_module(ng->cfg);
	return 1;
     }
   else if (ng->item_active && ev->buttons == 1)
     {
	it->drag.x = ev->root.x;
	it->drag.y = ev->root.y;
	it->drag.start = 1;
	it->drag.dnd = 0;
	ng->item_drag = it;
     }
   else if (ev->buttons == 3)
     {
	edje_object_signal_emit(ng->label, "e,state,label_hide", "e");
     }

   ITEM_MOUSE_DOWN(it, ev);

   if (ng->cfg->stacking != on_desk)
     evas_event_feed_mouse_down
       (ng->win->evas, ev->buttons, 0, ev->timestamp, NULL);

   if (ng->menu_wait_timer)
     ecore_timer_del(ng->menu_wait_timer);
   ng->menu_wait_timer =
     ecore_timer_add(0.1, _ngi_menu_wait_timer_cb, ng);  

   return 1;
}


static Eina_Bool
_ngi_win_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Ng *ng = data;
  
   if ((ev->event_window != ng->win->input) &&
       (ev->event_window != ng->win->edge))
     return 1;

   if (ng->item_active)
     {
	ITEM_MOUSE_UP(ng->item_active, ev);

	if (ng->cfg->stacking != on_desk)
	  evas_event_feed_mouse_up
	    (ng->win->evas, ev->buttons, 0, ev->timestamp, NULL);

	if (ng->item_drag)
	  {
	     ng->item_drag->drag.start = 0;
	     ng->item_drag = NULL;
	  }
     }

   return 1;
}

static Eina_Bool
_ngi_win_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Wheel *ev = event;
   Ng *ng = data;

   if ((ev->event_window != ng->win->input) &&
       (ev->event_window != ng->win->edge))
     return 1;

   if (ng->cfg->stacking != on_desk)
     evas_event_feed_mouse_wheel
       (ng->win->evas, 0, ev->z, ev->timestamp, NULL);

   return 1;
}


static Eina_Bool 
_ngi_win_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Move *ev = event;
   Ng *ng = data;

   if ((ev->event_window != ng->win->input) &&
       (ev->event_window != ng->win->edge))
     return 1;

   int pos = ng->horizontal ? ev->root.x : ev->root.y;

   pos -= ng->horizontal ? ng->zone->x : ng->zone->y;
  
   if (ng->pos != pos)
     {
	if (pos >= ng->start && pos < ng->start + ng->w)
	  {
	     evas_event_feed_mouse_move
	       (ng->win->evas,
		ev->root.x - ng->win->x,
		ev->root.y - ng->win->y,
		0, NULL);

	     ng->pos = pos;
	     ng->zoom_out = 0;
	  }
	else 
	  {
	     ng->zoom_out = 1;
	  }
     }
   
   ng->changed = 1;

   _ngi_item_activate(ng);

   if (ng->item_drag)
     {
	int  dx = ev->root.x - ng->item_drag->drag.x;
	int  dy = ev->root.y - ng->item_drag->drag.y;

	if (((dx * dx) + (dy * dy)) > (e_config->drag_resist * e_config->drag_resist))
	  {
	     ng->item_drag->drag.dnd = 1;
	     ng->item_drag->drag.start = 0;

	     ITEM_DRAG_START(ng->item_drag);
	     ITEM_MOUSE_OUT(ng->item_active);          

	     ng->item_active = NULL;
	     ng->item_drag = NULL;
	  }
     }

   ngi_animate(ng);
  
   return 1;
}

static void
_ngi_win_cb_drop_enter(void *data, const char *type, void *event_info)
{
   Ng *ng = data;
   ng->dnd = 1;
   ngi_mouse_in(ng);
}

static void
_ngi_win_cb_drop_leave(void *data, const char *type, void *event_info)
{
   Ng *ng = data;
   ng->dnd = 0;
   ngi_mouse_out(ng);
}

static void
_ngi_win_cb_drop_move(void *data, const char *type, void *event_info)
{
   /* Ng *ng; */
}

static void
_ngi_win_cb_drop_end(void *data, const char *type, void *event_info)
{
   Ng *ng = data;
   ng->dnd = 0;
   ngi_mouse_out(ng);
}


/**************************************************************************/

static int 
_ngi_zoom_function2(Ng *ng)
{
   double f, ff, sqrt_ffxx, sqrt_ff_1;

   f = 5.5;
   ff = f * f;
   sqrt_ffxx = sqrt(ff);
   sqrt_ff_1 = sqrt(ff - 1.0);

   return (int)(((ng->cfg->zoomfactor) * 
		 ((sqrt_ff_1 - sqrt_ffxx) / (sqrt_ff_1 - f)) + 1.0) * ng->size);
}

static void
_ngi_netwm_icon_geometry_set(E_Border *bd, int x, int y, int w, int h)
{
   unsigned int sizes[4];
   sizes[0] = x;
   sizes[1] = y;
   sizes[2] = w;
   sizes[3] = h;
   ecore_x_window_prop_card32_set
     (bd->win, ECORE_X_ATOM_NET_WM_ICON_GEOMETRY, sizes, 4);
}

void 
ngi_input_extents_calc(Ng *ng, int resize)
{
   Ngi_Box *box;
   Eina_List *l;
   int item_zoomed;
   Ngi_Win *win = ng->win;
   E_Zone *zone = ng->zone;

   int x, y, w, h;
   int wx, wy, ww, wh;
   


   int hidden = 0;

   if (ng->hide_state == hiding || ng->hide_state == hidden)
     hidden = ng->size + 15;

   if (ng->state != unzoomed && !ng->zoom_out)
     {      
	item_zoomed = _ngi_zoom_function2(ng);
     } 
   else
     {
	/* eeeek FIXME calc with edge distances + hidestep*/
	item_zoomed = ng->size + (ng->cfg->autohide ? 18 : 10); 
     }

   int extra = ng->size;

   switch (ng->cfg->orient)
     {
      case E_GADCON_ORIENT_BOTTOM:
	 x = ng->start - extra;
	 y = (ng->win->h + hidden) - item_zoomed;
	 ww = w = ng->w + 2*extra;
	 h = item_zoomed;

	 wx = zone->x + x;
	 wy = zone->y + (zone->h - 4);
	 wh = 4;
	 break;
      case E_GADCON_ORIENT_TOP:
	 x = ng->start;
	 y = - hidden;
	 ww = w = ng->w;
	 h = item_zoomed;
	 
	 wx = zone->x + x;
	 wy = zone->y;
	 wh = 4;
	 break;
      case E_GADCON_ORIENT_LEFT:
	 x = - hidden;
	 y = ng->start;
	 w = item_zoomed;
	 wh = h = ng->w;

	 wx = zone->x;
	 wy = zone->y + y;
	 ww = 4;
	 break;
      case E_GADCON_ORIENT_RIGHT:
	 x = (ng->win->w + hidden) - item_zoomed;
	 y = ng->start;
	 w = item_zoomed;
	 wh = h = ng->w;

	 wx = zone->x + (ng->zone->w - 4);
	 wy = zone->y + y;
	 ww = 4;
	 break;
     }

   if (win->edge && resize) 
     {
       ecore_x_window_move_resize(win->edge, wx, wy, ww, wh);
       e_drop_handler_geometry_set(win->drop_handler, wx, wy, ww, wh);

     }
   
   ecore_x_window_move_resize(win->input,
			      ng->zone->x + win->x + x,
			      ng->zone->y + win->y + y,
			      w, h);

   if (ngi_config->use_composite && win->ee)
     {
	XRectangle rect;
	rect.x = rect.y = 0;
	rect.width = rect.height = 0;
	XShapeCombineRectangles((Display*)ecore_x_display_get(),
				win->evas_win, ShapeInput, 0, 0,
				&rect, 1, ShapeSet, Unsorted);
     }

   switch (ng->cfg->stacking)
     {
      case below_fullscreen:
	 e_container_window_raise(ng->zone->container, ng->win->evas_win, 150);
	 if (ng->cfg->lock_deskswitch)
	   e_container_window_raise(ng->zone->container, ng->win->edge, 999);
	 else
	   e_container_window_raise(ng->zone->container, ng->win->edge, 150);
	 e_container_window_raise(ng->zone->container, ng->win->input, 150);
	 break;
      case on_desk:
	 if (ng->cfg->lock_deskswitch)
	   e_container_window_raise(ng->zone->container, ng->win->edge, 999);
	 else
	   e_container_window_raise(ng->zone->container, ng->win->edge, 50);

	 e_container_window_raise(ng->zone->container, ng->win->input, 50);
	 break;
      case above_all:
	 e_container_window_raise(ng->zone->container, ng->win->input, 999);
     }

   EINA_LIST_FOREACH (ng->boxes, l, box)
     {
	if (!box->drop_handler) continue;

	int w = box->w;
	if (w < 10) w = 10;

	switch (ng->cfg->orient)
	  {
	   case E_GADCON_ORIENT_BOTTOM:
	      e_drop_handler_geometry_set(box->drop_handler,
					  ng->zone->x + box->pos, 
					  ng->zone->y + win->y,
					  w, win->h);
	      break;
	   case E_GADCON_ORIENT_TOP:
	      e_drop_handler_geometry_set(box->drop_handler,
					  ng->zone->x + box->pos, 
					  ng->zone->y,
					  w, item_zoomed);
	      break;
	   case E_GADCON_ORIENT_LEFT:
	      e_drop_handler_geometry_set(box->drop_handler,
					  ng->zone->x, 
					  ng->zone->y + box->pos,
					  item_zoomed, w);
	      break;
	   case E_GADCON_ORIENT_RIGHT:
	      e_drop_handler_geometry_set(box->drop_handler,
					  ng->zone->x + (ng->zone->w + hidden) - item_zoomed, 
					  ng->zone->y + box->pos,
					  item_zoomed, w);
	  }
     }
}


/**************************************************************************/

static Ngi_Item *
_ngi_item_at_position_get(Ng *ng)
{
   Eina_List *l, *ll;
   Ngi_Item *it;
   Ngi_Box *box;
   int size = ng->size/2;
   int pos = ng->pos;

   EINA_LIST_FOREACH (ng->boxes, ll, box)
     {
	if (pos < box->pos) continue;

	EINA_LIST_FOREACH (box->items, l, it)
	  {
	     if (it->scale == 0.0) continue;

	     if (pos <= it->pos + size + ng->item_spacing)
	       {
		  if (pos + ng->item_spacing < it->pos - size)
		    return NULL;
		  else
		    return it;
	       }
	  }
     }
   return NULL;
}


static void 
_ngi_item_activate(Ng *ng)
{
   Ngi_Item *it = _ngi_item_at_position_get(ng);

   if (it && ng->cfg->zoom_one) ng->pos = it->pos;
  
   if (ng->mouse_in && it && it != ng->item_active)
     {
	ITEM_MOUSE_OUT(ng->item_active);
	ITEM_MOUSE_IN(it);
	ng->item_active = it;
     }
   else if (!it || ng->zoom_out)
     {
	ITEM_MOUSE_OUT(ng->item_active);
	ng->item_active = NULL; /*XXX TEST*/
     }

   if (ng->item_active && ng->cfg->show_label)
     {
	/* XXX this duplicates code from ngi_redraw */
	int h, size;
	double zoom = 1.0, disp;
      
	if (ng->win->ee)
	  h = ng->win->h;
	else
	  h = ng->zone->h;

	if (ng->zoom > 1.0)
	  _ngi_zoom_function(ng, 0, &zoom, &disp);
      
	size = zoom * ng->size;
	switch (ng->cfg->orient)
	  {
	   case E_GADCON_ORIENT_BOTTOM:
	      evas_object_move(ng->label, ng->item_active->pos, 
			       (h + ng->hide_step) - (size + 12 + TEXT_DIST)); /*XXX 12?*/
	      break;
	   case E_GADCON_ORIENT_TOP:
	      evas_object_move(ng->label, ng->item_active->pos, 
			       (size + 12 + TEXT_DIST) - ng->hide_step);
	  }    
 
	if (ng->item_active->label && ng->item_active->label[0])
	  {
	     evas_object_show(ng->label);
	     edje_object_signal_emit(ng->label, "e,state,label_show", "e");
	     edje_object_part_text_set(ng->label, "e.text.label", ng->item_active->label);
	  }
	else
	  evas_object_hide(ng->label);
     }
}


/**************************************************************************/

static void 
_ngi_item_appear(Ng *ng, Ngi_Item *it)
{
   double appear_duration = 0.1;

   it->scale = (ecore_time_get() - it->start_time) / appear_duration;

   if (it->scale < 1.0)
     {
	ngi_reposition(ng);
     }
   else
     {
	it->scale = 1.0;
	it->state = normal;

	ng->items_scaling = eina_list_remove(ng->items_scaling, it);

	ngi_reposition(ng);      
	ngi_input_extents_calc(ng, 1);
     }
  
   _ngi_item_activate(ng);
}

static void 
_ngi_item_disappear(Ng *ng, Ngi_Item *it)
{
   double appear_duration = 0.2;

   double delta = it->scale - (1.0 - (ecore_time_get() - it->start_time) / appear_duration);

   it->scale -= delta;

   if (it->scale > 0.0)
     {
	ngi_reposition(ng);
     }
   else
     {
	ng->items_scaling = eina_list_remove(ng->items_scaling, it);

	evas_object_clip_unset(it->obj);
	evas_object_clip_unset(it->over);

	it->scale = 0.0;

	if (ng->item_active == it) ng->item_active = NULL;
      
	if (it->visible) ITEM_FREE(it); /* FIXME hack */

	ng->size = ng->cfg->size;

	ngi_reposition(ng);
	ngi_input_extents_calc(ng, 1);
	ngi_animate(ng);

	ngi_hide_clip(ng);
     }

   _ngi_item_activate(ng); /* XXX huh?! */
}

static void 
_ngi_zoom_in(Ng *ng)
{
   Config_Item *cfg = ng->cfg;
 
   ng->zoom = ng->zoom +
     (cfg->zoomfactor / (double)e_config->framerate) / cfg->zoom_duration; 

   if (ng->zoom < cfg->zoomfactor)
     {
	if (ng->state != zooming)
	  {
	     ng->state = zooming;
          
	     ngi_input_extents_calc(ng, 0);
	  }
     }
   else
     {
	ng->zoom = cfg->zoomfactor;
	ng->state = zoomed;
     }
}

static void 
_ngi_zoom_out(Ng *ng)
{
   Config_Item *cfg = ng->cfg;

   ng->zoom = ng->zoom -
     (cfg->zoomfactor / (double)e_config->framerate) / cfg->zoom_duration; 

   if (ng->zoom > 1.0)
     {
	ng->state = unzooming;
     }
   else
     {
	ng->zoom = 1.0;
	ng->state = unzoomed;
	ngi_input_extents_calc(ng, 0);
	if (ng->item_active)
	  {
	     ITEM_MOUSE_OUT(ng->item_active);
	     ng->item_active = NULL;
	  }
	edje_object_signal_emit(ng->label, "e,state,label_hide", "e"); 
     }
}

static int 
_ngi_autohide(Ng *ng, int hide)
{
   Config_Item *cfg = ng->cfg;
   int step;
   int size;

   if (hide)
     {
       if (ng->hide_state == hiding)
	 edje_object_signal_emit(ng->label, "e,state,label_hide", "e"); 
       ng->hide_state = hiding;
     }
   else if (ng->hide_state != showing)
     {
	ngi_bar_show(ng);
	//evas_object_show(ng->clip);
	ng->hide_state = showing;
     }

   size = ng->size + 15;

   step = ((double)(size * 2) / (double)e_config->framerate) / cfg->zoom_duration;

   if (step < 1) step = 1;

   if (ng->hide_state == hiding)
     {
	ng->hide_step += step;
	if (ng->hide_step > size) 
	  {
	     ng->hide_step = size;
	     goto end;
	  }
     }
   else
     {
	ng->hide_step -= step;
	if (ng->hide_step < 0)
	  {
	     ng->hide_step = 0;
	     goto end;	
	  }
     }

   return 1;

 end:
   if (hide)
     {
	if (ngi_config->use_composite) ngi_bar_hide(ng);
	ng->hide_state = hidden;
	ng->zoom = 1.0;
	ng->state = unzoomed;
	ng->item_active = NULL;
     }
   else
     ng->hide_state = show;

   ngi_input_extents_calc(ng, 0);
   return 0;
}

void 
ngi_animate(Ng *ng)
{
   ng->changed = 1;
   if (ng->animator) return;
   ng->animator = ecore_timer_add(1.0/(double)e_config->framerate*2, _ngi_animator, ng);
}

static Eina_Bool 
_ngi_animator(void *data)
{
   Ng *ng = (Ng*) data;
   int cont = 0; /* continue */
   Eina_List *l;
   Ngi_Item *it;

   for(l = ng->items_scaling; l;)
     {
	it = (Ngi_Item*) l->data;
	l = l->next;
     
	if (it->state == appearing)
	  _ngi_item_appear(ng, it);
	else if (it->state == disappearing)
	  _ngi_item_disappear(ng, it);

	cont++;
     }

   if (ng->cfg->zoomfactor > 1.0)
     {
	if (!ng->zoom_out)
	  {
	     if (ng->state != zoomed)
	       {
		  _ngi_zoom_in(ng);
		  cont++;
	       }
	  }
	else
	  {
	     if (ng->state != unzoomed)
	       {
		  _ngi_zoom_out(ng);
		  cont++;
	       }
	  }
     }
  
   if (ng->cfg->autohide)
     {
	if (ng->mouse_in || ng->show_bar || ng->menu_wait_timer)
	  {
	     if (ng->hide_state != show)
	       {
		  _ngi_autohide(ng, 0);
		  cont++;
	       }
	  }
	else
	  {
	     if (ng->hide_state != hidden)
	       {
		  _ngi_autohide(ng, 1);
		  cont++;
	       }
	  }
     }
  
   cont += ng->changed;

   if (cont)
     _ngi_redraw(ng);
   else
     ng->animator = NULL;
  
   return cont;
}


void 
ngi_reposition(Ng *ng)
{
   Ngi_Box *box;
   double pos;
   Eina_List *l, *ll;
   Ngi_Item *it;
   int size = ng->size;
   int cnt = 0;

   ng->w = 0;

   EINA_LIST_FOREACH (ng->boxes, l, box)
     {
	box->w = 0;
      
	EINA_LIST_FOREACH (box->items, ll, it)
	  {
	     if (it->scale == 0.0) continue;

	     box->w += size * it->scale + ng->item_spacing;
	  }
        
	ng->w += box->w;
	if (cnt++ > 0) ng->w += ng->separator_width * 2 + ng->item_spacing*6;
     }

   ng->start =  0.5 * ((ng->horizontal ? ng->win->w : ng->win->h) - ng->w);

   double zoom, disp;

   int middle = (ng->horizontal ? ng->win->w : ng->win->h)/2;
  
   double distance = (double)(ng->start - middle) / (ng->size + ng->item_spacing);
   double tmp = ng->zoom;
   ng->zoom = ng->cfg->zoomfactor;
   _ngi_zoom_function(ng, distance, &zoom, &disp);
   ng->zoom = tmp;
   int end = (int)(middle + disp);

   /* shrink bar when it becomes larger than screen height/width  */
   if (end < 0 && size > 1)
     {
	ng->size -=1;
	ngi_reposition(ng);
	return;
     }

   cnt = 0;
   pos = ng->start;

   EINA_LIST_FOREACH (ng->boxes, l, box)
     {
	box->pos = pos;

	if (cnt++ > 0) pos += (ng->separator_width * 2 + ng->item_spacing*6);

	EINA_LIST_FOREACH (box->items, ll, it)
	  {
	     if (it->scale == 0.0) continue;

	     pos += 0.5 * (it->scale * size + ng->item_spacing);
	     it->pos = (int) pos;
	     pos += 0.5 * (it->scale * size + ng->item_spacing);
	  }
     }

   if (ng->items_scaling) return;
   
   if (ng->cfg->ecomorph_features)
     { 
	EINA_LIST_FOREACH (ng->boxes, l, box)
	  {
	     if (!(box->cfg->type == taskbar)) continue;;

	     switch(ng->cfg->orient)
	       {
		case E_GADCON_ORIENT_BOTTOM:
		   for (ll = box->items; ll; ll = ll->next)
		     {
			it = (Ngi_Item*) ll->data;
			_ngi_netwm_icon_geometry_set(it->border, it->pos - size/2,
						     (ng->win->y + ng->win->h) - size,
						     size, size);
		     }
		   break;
		case E_GADCON_ORIENT_TOP:
		   for (ll = box->items; ll; ll = ll->next)
		     {
			it = (Ngi_Item*) ll->data;
			_ngi_netwm_icon_geometry_set(it->border, it->pos - size/2, 
						     ng->hide_step, size, size);
		     }
		   break;
		case E_GADCON_ORIENT_LEFT:
		   for (ll = box->items; ll; ll = ll->next)
		     {
			it = (Ngi_Item*) ll->data;
			_ngi_netwm_icon_geometry_set(it->border, ng->hide_step,
						     it->pos - size/2, size, size);
		     }
		   break;
		case E_GADCON_ORIENT_RIGHT:
		   for (ll = box->items; ll; ll = ll->next)
		     {
			it = (Ngi_Item*) ll->data;
			_ngi_netwm_icon_geometry_set(it->border, 
						     (ng->win->x + ng->win->w) - size, 
						     it->pos - size/2, size, size);
		     }
		   break;
	       }
	  }
     }
}


static void 
_ngi_zoom_function(Ng *ng, double d, double *zoom, double *disp)
{
   double  range, f, x, ff, sqrt_ffxx, sqrt_ff_1;

   Config_Item *cfg = ng->cfg;

   range = ng->cfg->zoom_one ? 1.0 : cfg->zoom_range;
   f = 5.5;
   x = d / range;

   ff = f * f;
   sqrt_ffxx = sqrt(ff - x * x);
   sqrt_ff_1 = sqrt(ff - 1.0);

   double z = cfg->zoomfactor * (double)ng->size;
   double w = range *  (double)ng->size;
   if (d > -range && d < range)
     {
	*zoom = (ng->zoom - 1.0) * (cfg->zoomfactor - 1.0) *
	  ((sqrt_ff_1 - sqrt_ffxx) / (sqrt_ff_1 - f)) + 1.0;

	/// hack for tighter packing ///
	float z = 0.0f;
	if (ng->state != unzoomed )
	  {
	     z = fabsf(((ng->zoom) - (*zoom)) / (ng->zoom - 1.0f) ); 
	     if (z > 1.0f) 	  
	       z = (float) ng->item_spacing;
	     else
	       z *= (float) ng->item_spacing;
	  }
	else
	  {
	     z = (float) ng->item_spacing;
	  }
       
	*disp = (((float)ng->size) + z)
	  * ((ng->zoom - 1.0) * (cfg->zoomfactor - 1.0)
	     * (range * (x * (2 * sqrt_ff_1 - sqrt_ffxx) -
			 ff * atan(x / sqrt_ffxx)) / (2.0 * (sqrt_ff_1 - f))) + d);
     }
   else
     {
	*disp = ((z * w) + ((- z * w) / - w) * (d - w));
 
	*zoom = 1.0;
	*disp = (ng->size + ng->item_spacing) *
	  ((ng->zoom - 1.0) * (cfg->zoomfactor - 1.0) *
	   (range * (sqrt_ff_1 - ff * atan(1.0 / sqrt_ff_1)) /
	    (2.0 * (sqrt_ff_1 - f))) + range + fabs(d) - range);
     
	if (d < 0.0)
	  *disp = -(*disp);
     }
}

static void 
_ngi_redraw(Ng *ng)
{
   double zoom2, disp2, distance, pos;	      
   int end1, end2, size, size_spacing, spacing, hide_step, bg_dist1, bg_dist2;
   Ngi_Item *it;
   Ngi_Box *box;
   Eina_List *l, *ll;

   Config_Item *cfg = ng->cfg;

   int bg_border = 10;
   int edge_spacing = 6;
   int in_offset = 16;
   int cnt = 0;
   double disp = 0.0;
   double zoom = 1.0;
  
   spacing = ng->item_spacing;
   hide_step = cfg->autohide ? ng->hide_step : 0;
   size_spacing = ng->size + spacing;
   bg_dist1 = edge_spacing - bg_border - in_offset;
   bg_dist2 = edge_spacing + ng->size + bg_border;

   /* do not zoom */
   if (cfg->zoomfactor < 1.2)
     ng->pos = ng->win->w / 2;

   int x, y, w, h;
   if (ng->win->ee)
     {
	w = ng->win->w;
	h = ng->win->h;
	x = 0;
	y = 0;
     }
   else
     {
	w = ng->zone->w;
	h = ng->zone->h;
	x = ng->zone->x;
	y = ng->zone->y;
     }

   if (cfg->show_background)
     {
	if (cfg->zoomfactor > 1.0)
	  {
	     distance = (double)(ng->start - ng->pos) / size_spacing;
	     _ngi_zoom_function(ng, distance, &zoom, &disp);
	     end1 = (int)(ng->pos + disp);
	  }
	else 
	  {
	     end1 = ng->start;
	  }

	if (cfg->zoomfactor > 1.0)
	  {
	     distance = (double)(ng->start + ng->w - ng->pos) / size_spacing;
	     _ngi_zoom_function(ng, distance, &zoom, &disp);
	     end2 = (int)(ng->pos + disp);
	  }
	else
	  {
	     end2 = ng->start + ng->w;
	  }

	switch (cfg->orient)
	  {
	   case E_GADCON_ORIENT_BOTTOM:
	      evas_object_move(ng->o_bg, x + (end1 - bg_border), y + ((h + hide_step) - bg_dist2));
	      evas_object_resize(ng->o_bg, (end2 - end1) + bg_border*2 + spacing, bg_dist2 - bg_dist1);
	      evas_object_move(ng->o_frame, x + (end1 - bg_border), y + ((h + hide_step) - bg_dist2));
	      evas_object_resize(ng->o_frame, (end2 - end1) + bg_border*2 + spacing, bg_dist2 - bg_dist1);
	      break;

	   case E_GADCON_ORIENT_TOP:
	      evas_object_move(ng->o_bg, x + (end1 - bg_border), y + (bg_dist1 - hide_step));
	      evas_object_resize(ng->o_bg, (end2 - end1) + bg_border*2 + spacing, bg_dist2 - bg_dist1);
	      evas_object_move(ng->o_frame, x + (end1 - bg_border), y + (bg_dist1 - hide_step));
	      evas_object_resize(ng->o_frame, (end2 - end1) + bg_border*2 + spacing, bg_dist2 - bg_dist1);
	      break;

	   case  E_GADCON_ORIENT_LEFT:
	      evas_object_move(ng->o_bg, x + (bg_dist1 - hide_step), y + (end1 - bg_border));
	      evas_object_resize(ng->o_bg, bg_dist2 - bg_dist1, (end2 - end1) + (bg_border * 2));
	      evas_object_move(ng->o_frame, x + (bg_dist1 - hide_step), y + (end1 - bg_border));
	      evas_object_resize(ng->o_frame, bg_dist2 - bg_dist1, (end2 - end1) + (bg_border * 2));
	      break;

	   case E_GADCON_ORIENT_RIGHT:
	      evas_object_move(ng->o_bg, x + ((w + hide_step) - bg_dist2), y + (end1 - bg_border));
	      evas_object_resize(ng->o_bg, bg_dist2 - bg_dist1, (end2 - end1) + (bg_border * 2));
	      evas_object_move(ng->o_frame, x + ((w + hide_step) - bg_dist2), y + (end1 - bg_border));
	      evas_object_resize(ng->o_frame, bg_dist2 - bg_dist1, (end2 - end1) + (bg_border * 2));
	  }
     }
  
   if (ng->item_active && ng->state != unzoomed)
     {
	if (cfg->zoomfactor > 1.0)
	  {
	     _ngi_zoom_function(ng, 0, &zoom, &disp);
	  }
      
	size = zoom * ng->size;
	switch (cfg->orient)
	  {
	   case E_GADCON_ORIENT_BOTTOM:
	      evas_object_move(ng->label, ng->item_active->pos, (h + hide_step) - (size + edge_spacing*2 + TEXT_DIST));
	      break;
	   case E_GADCON_ORIENT_TOP:
	      evas_object_move(ng->label, ng->item_active->pos, (size + edge_spacing*2 + TEXT_DIST) - hide_step);
	  }    
     }
  
   EINA_LIST_FOREACH (ng->boxes, ll, box)
     {
	if (cnt++ > 0)
	  {
	     distance = (double)(box->pos + ng->separator_width + ng->item_spacing*3 - ng->pos) / size_spacing;
	  
	     _ngi_zoom_function(ng, distance, &zoom, &disp);

	     pos = (ng->pos + disp);

	     switch (cfg->orient)
	       {
		case E_GADCON_ORIENT_BOTTOM:
		   evas_object_move(box->separator, (x + pos) - ng->separator_width/2, y + ((h + hide_step) - bg_dist2));
		   evas_object_resize(box->separator, ng->separator_width - 1, bg_dist2 - bg_dist1);
		   break;

		case E_GADCON_ORIENT_TOP:
		   evas_object_move(box->separator, (x + pos) - ng->separator_width/2, y + (bg_dist1 - hide_step));
		   evas_object_resize(box->separator, ng->separator_width - 1, bg_dist2 - bg_dist1);
		   break;

		case E_GADCON_ORIENT_LEFT:
		   evas_object_move(box->separator, x + (bg_dist1 - hide_step), (y + pos) - ng->separator_width/2);
		   evas_object_resize(box->separator, bg_dist2 - bg_dist1, ng->separator_width - 1);
		   break;

		case E_GADCON_ORIENT_RIGHT:
		   evas_object_move(box->separator, x + ((w + hide_step) - bg_dist2), (y + pos) - ng->separator_width/2);
		   evas_object_resize(box->separator, bg_dist2 - bg_dist1, ng->separator_width - 1);

	       }
	     evas_object_show(box->separator);
	  }
	else evas_object_hide(box->separator);

	EINA_LIST_FOREACH (box->items, l, it)
	  {
	     if (cfg->zoomfactor > 1.0)
	       {
		  distance = (double)(it->pos - ng->pos) / (double)size_spacing;

		  _ngi_zoom_function(ng, distance, &zoom, &disp);
	  
		  size = (int)((it->scale * zoom) * ng->size);
	  
		  pos = (ng->pos + disp) - (size / 2);

		  /* hack for tighter packing -> dont let icons overlap */
		  if (zoom > 1.0) 
		    {
		       if (it->pos < ng->pos)
			 {
			    _ngi_zoom_function(ng, distance - 1.0, &zoom2, &disp2);
			    if (zoom2 == 1.0 && (ng->pos + disp2) + ng->size/2  > (ng->pos + disp) - size/2)
			      pos += (((double)ng->pos + disp2) + (double)ng->size/2.0) - (((double)ng->pos + disp) - (double)size/2.0);		  
			 }
		       else if (it->pos > ng->pos)
			 {
			    _ngi_zoom_function(ng, distance + 1.0, &zoom2, &disp2);
			    if (zoom2 == 1.0 && (ng->pos + disp2) - ng->size/2  < (ng->pos + disp) + size/2)
			      pos -= (((double)ng->pos + disp) + (double)size/2.0) - (((double)ng->pos + disp2) - (double)ng->size/2.0);		  
			 }
		    }
	       }
	     else
	       {
		  pos = it->pos - ng->size/2;
		  size = it->scale * ng->size;
	       }
	  
	     switch (cfg->orient)
	       {
		case E_GADCON_ORIENT_BOTTOM:
		   evas_object_move(it->obj,  x + pos, y + ((h + hide_step) - (size + edge_spacing)));
		   if (!it->overlay_signal_timer) 
		     evas_object_move(it->over, x + pos, y + ((h + hide_step) - (size + edge_spacing)));
		   break;

		case E_GADCON_ORIENT_TOP:
		   evas_object_move(it->obj,  x + pos, y + (edge_spacing - hide_step));
		   if (!it->overlay_signal_timer) 
		     evas_object_move(it->over, x + pos, y + (edge_spacing - hide_step));
		   break;

		case E_GADCON_ORIENT_LEFT:
		   evas_object_move(it->obj,  x + (edge_spacing - hide_step), y + pos);
		   if (!it->overlay_signal_timer) 
		     evas_object_move(it->over, x + (edge_spacing - hide_step), y + pos);
		   break;

		case E_GADCON_ORIENT_RIGHT:
		   evas_object_move(it->obj,  x + ((w + hide_step) - (size + edge_spacing)), y + pos);
		   if (!it->overlay_signal_timer) 
		     evas_object_move(it->over, x + ((w + hide_step) - (size + edge_spacing)), y + pos);
	       }

	     if (size != it->size)
	       {
		  it->size = size;
		  evas_object_resize(it->obj, size, size);
		  if (!it->overlay_signal_timer) 
		    evas_object_resize(it->over, size, size);
	       }
	  }
     }
   ng->changed = 0;
}

void 
ngi_freeze(Ng *ng)
{
   if (ng->animator)
     {
	ecore_timer_del(ng->animator);
	ng->animator = NULL;
     }
}


void
ngi_hide_clip(Ng *ng)
{
   Eina_List *l;
   int found = 0;
   Ngi_Box *box;
  
   /* hide items clip if there are no items */
   EINA_LIST_FOREACH (ng->boxes, l, box)
     {
	if (box->items)
	  {
	     found = 1;
	     break;
	  }
     }
   if (found)
     ngi_bar_show(ng);
   else
     ngi_bar_hide(ng);

}

void 
ngi_thaw(Ng *ng)
{
   ngi_hide_clip(ng);
   ngi_reposition(ng);
   ngi_input_extents_calc(ng, 1);
   ngi_animate(ng);
}

void ngi_bar_show(Ng *ng)
{
   if (ng->boxes && !evas_object_visible_get(ng->clip))
     evas_object_show(ng->clip);
}

void ngi_bar_hide(Ng *ng)
{
   if (evas_object_visible_get(ng->clip))
     evas_object_hide(ng->clip);
}

