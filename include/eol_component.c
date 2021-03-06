#include "eol_component_button.h"
#include "eol_component_list.h"
#include "eol_component_actor.h"
#include "eol_component_entry.h"
#include "eol_component_percent_bar.h"
#include "eol_component_check.h"
#include "eol_component.h"
#include "eol_logger.h"
#include "eol_font.h"
#include "eol_sprite.h"
#include "eol_mouse.h"
#include "eol_draw.h"
#include "eol_config.h"
#include <glib/gstring.h>

/*local types*/
typedef struct
{
  GString  * buffer;
  eolUint    justify;
  eolBool    wordWrap;
  eolUint    fontSize;
  eolVec3D   color;
  eolFloat   alpha;
  eolFont  * font;    /**<if defined, it will use the custom font to draw text*/
}eolComponentLabel;

typedef struct
{
  eolSprite *image;
  eolUint    frame;
  eolBool    scaleToBounds;
}eolComponentImage;

typedef struct
{
  eolUint    sliderType;
  eolSprite *slider;
  eolSprite *sliderHigh;
  eolSprite *bar;
  eolSprite *cap1;
  eolSprite *cap2;
  eolVec3D   barColor;
  eolVec3D   sliderColor;
  eolBool    vertical;
  eolFloat   position;
  eolFloat   oldPosition;
  eolUint    sliderState;
}eolComponentSlider;

/*local global variables*/
eolSprite * _eol_component_slider[4] = {NULL,NULL,NULL,NULL};
eolLine     _eol_component_slider_files[4];
eolVec3D    _eol_component_label_color = {1,1,1};
eolFloat    _eol_component_label_alpha = 1;
eolUint     _eol_component_label_font_size = 1;

/*local function prototypes*/
void eol_component_label_new(eolComponent *component);
void eol_component_label_free(eolComponent *component);
void eol_component_label_move(eolComponent *component,eolRect newbounds);
void eol_component_slider_new(eolComponent *component);

eolBool eol_component_has_changed(eolComponent *component);
eolInt eol_component_get_state(eolComponent *component);

void eol_component_textinput_new(eolComponent *component);
void eol_component_textinput_get_text(eolComponent *component,char *text);


void eol_component_get_rect_from_bounds(eolRect *rect,eolRect canvas, eolRectFloat bounds);

void eol_component_label_free(eolComponent *component);
void eol_component_slider_free(eolComponent *component);
void eol_component_image_free(eolComponent *component);

void eol_component_make_label(
    eolComponent * component,
    char         * text,
    eolUint        justify,
    eolInt         fontSize,
    eolBool        wordWrap,
    char         * fontName,
    eolVec3D       color,
    eolFloat       alpha
  );

/*definitions*/

void eol_label_configure(eolConfig *conf)
{
  if (!conf)return;
  eol_config_get_vec3d_by_tag(&_eol_component_label_color,conf,"label_color");
  eol_config_get_float_by_tag(&_eol_component_label_alpha,conf,"label_alpha");
  eol_config_get_uint_by_tag(&_eol_component_label_font_size,conf,"label_size");
}

void eol_component_config()
{
  eolLine buf;
  eolLine sliderVfile,sliderVhighfile,sliderHfile,sliderHhighfile;
  eolConfig *conf = NULL;
  
  eol_line_cpy(sliderVfile,"images/UI/slider_v.png");
  eol_line_cpy(sliderVhighfile,"images/UI/slider_v_high.png");
  eol_line_cpy(sliderHfile,"images/UI/slider.png");
  eol_line_cpy(sliderHhighfile,"images/UI/slider_high.png");
  
  conf = eol_config_load("system/component.cfg");
  if (conf != NULL)
  {
    eol_label_configure(conf);
    eol_button_configure(conf);
    eol_check_configure(conf);
    eol_config_get_line_by_tag(buf,conf,"slider_verticle");
    if (strlen(buf) > 0)
    {
      eol_line_cpy(sliderVfile,buf);
    }
    eol_config_get_line_by_tag(buf,conf,"slider_verticle_high");
    if (strlen(buf) > 0)
    {
      eol_line_cpy(sliderVhighfile,buf);
    }
    eol_config_get_line_by_tag(buf,conf,"slider_horizontal");
    if (strlen(buf) > 0)
    {
      eol_line_cpy(sliderHfile,buf);
    }
    eol_config_get_line_by_tag(buf,conf,"slider_horizontal_high");
    if (strlen(buf) > 0)
    {
      eol_line_cpy(sliderHhighfile,buf);
    }
    
    eol_config_free(&conf);
  }

  _eol_component_slider[0] = eol_sprite_load(sliderVfile,-1,-1);
  _eol_component_slider[1] = eol_sprite_load(sliderVhighfile,-1,-1);
  _eol_component_slider[2] = eol_sprite_load(sliderHfile,-1,-1);
  _eol_component_slider[3] = eol_sprite_load(sliderHhighfile,-1,-1);
  eol_line_cpy(_eol_component_slider_files[0],sliderVfile);
  eol_line_cpy(_eol_component_slider_files[1],sliderVhighfile);
  eol_line_cpy(_eol_component_slider_files[2],sliderHfile);
  eol_line_cpy(_eol_component_slider_files[3],sliderHhighfile);
  
}

eolComponent * eol_component_new()
{
  eolComponent *component = NULL;
  component = (eolComponent *)malloc(sizeof(eolComponent));
  if (component == NULL)
  {
    eol_logger_message(
      EOL_LOG_ERROR,
      "eol_component: failed to allocate new component!");
    return NULL;
  }
  memset(component,0,sizeof(eolComponent));
  return component;
}

void eol_component_free(eolComponent **component)
{
  if ((!component)||(!*component))return;
  if ((*component)->data_free != NULL)
  {
    (*component)->data_free(*component);
  }
  free(*component);
  *component = NULL;
}

void eol_component_move(eolComponent *component,eolRect newbounds)
{
  if (!component)return;
  if (component->data_move != NULL)
  {
    component->data_move(component,newbounds);
  }
  else
  {
    eol_component_get_rect_from_bounds(&component->bounds,newbounds,component->rect);
  }
}

eolBool eol_component_update(eolComponent *component)
{
  if (!component)return eolFalse;
  if (component->data_update != NULL)
  {
    return component->data_update(component);
  }
  return eolFalse;
}

eolBool eol_component_changed(eolComponent *component)
{
  if (!component)return eolFalse;
  if (component->data_changed == NULL)return eolFalse;
  return component->data_changed(component);
}

void eol_component_set_focus(eolComponent *component,eolBool focus)
{
  if (!component)return;
  if (!component->canHasFocus)return;
  component->hasFocus = focus;
}

eolComponentLabel *eol_component_get_label_data(eolComponent *component)
{
  if ((!component)||
    (!component->componentData)||
    (component->type != eolLabelComponent))
  {
    return NULL;
  }
  return (eolComponentLabel*)component->componentData;
}

eolComponentImage *eol_component_get_image_data(eolComponent *component)
{
  if ((!component)||
    (!component->componentData)||
    (component->type != eolImageComponent))
  {
    return NULL;
  }
  return (eolComponentImage*)component->componentData;
}

eolComponentSlider *eol_component_get_slider_data(eolComponent *component)
{
  if ((!component)||
    (!component->componentData)||
    (component->type != eolSliderComponent))
  {
    return NULL;
  }
  return (eolComponentSlider*)component->componentData;
}

/*
 **** Free ****
*/

void eol_component_label_free(eolComponent *component)
{
  eolComponentLabel *label = eol_component_get_label_data(component);
  if (label == NULL)return;
  if (label->buffer != NULL)g_string_free(label->buffer,0);
  if (label->font != NULL)eol_font_free(&label->font);
  free(label);
  label = NULL;
  component->componentData = NULL;
}

void eol_component_slider_free(eolComponent *component)
{
  eolComponentSlider *slider = eol_component_get_slider_data(component);
  if (slider == NULL)return;
  eol_sprite_free(&slider->slider);
  eol_sprite_free(&slider->sliderHigh);
  eol_sprite_free(&slider->bar);
  eol_sprite_free(&slider->cap1);
  eol_sprite_free(&slider->cap2);
  free(slider);
  component->componentData = NULL;
}

void eol_component_image_free(eolComponent *component)
{
  eolComponentImage * image = eol_component_get_image_data(component);
  if (image == NULL)return;
  eol_sprite_free(&image->image);
  free(image);
  component->componentData = NULL;
}

/*


**** DRAW ****


*/


void eol_component_slider_draw(eolComponent *component)
{
  eolRect r;
  eolVec2D drawPosition;
  eolComponentSlider *slider = eol_component_get_slider_data(component);
  if (slider == NULL)return;
  eol_rect_copy(&r,component->bounds);

  if (slider->vertical)
  {
    r.w = component->bounds.w * 0.5;
    r.x += r.w * 0.5;
    drawPosition.x = r.x + (r.w * 0.5);
    drawPosition.y = r.y + (r.h * slider->position);
  }
  else
  {
    r.h = component->bounds.h * 0.5;
    r.y += r.h * 0.5;
    drawPosition.y = r.y + (r.h * 0.5);
    drawPosition.x = r.x + (r.w * slider->position);
  }
  
  eol_draw_solid_rect(r,slider->barColor,1);
  eol_draw_rect(r,eol_vec3d(1,1,1),1);
  if (slider->sliderState == eolSliderIdle)
  {
    if (slider->slider != NULL)
    {
      drawPosition.x -= slider->slider->frameWidth / 2;
      drawPosition.y -= slider->slider->frameHeight / 2;
      eol_sprite_draw(slider->slider,0,drawPosition.x,drawPosition.y);
    }
    else
    {
      /*TODO: draw a slider*/
    }
  }
  else
  {
    if (slider->sliderHigh != NULL)
    {
      drawPosition.x -= slider->sliderHigh->frameWidth / 2;
      drawPosition.y -= slider->sliderHigh->frameHeight / 2;
      eol_sprite_draw(slider->sliderHigh,0,drawPosition.x,drawPosition.y);
    }
    else
    {
      /*TODO: draw a slider*/
    }
  }
}


void eol_component_label_draw(eolComponent *component)
{
  eolComponentLabel *label = eol_component_get_label_data(component);
  if (label == NULL)return;
  if (label->buffer == NULL)return;
  if (label->font == NULL)
  {
    if (label->wordWrap)
    {
      eol_font_draw_text_block(
      label->buffer->str,
      component->bounds.x,
      component->bounds.y,
      component->bounds.w,
      0,
      label->color,
      label->alpha,
      label->fontSize
      );
    }
    else
    {
      eol_font_draw_text_justify(
        label->buffer->str,
        component->bounds.x,
        component->bounds.y,
        label->color,
        label->alpha,
        label->fontSize,
        label->justify
      );
    }
  }
  else
  {
    if (label->wordWrap)
    {
      eol_font_draw_text_block_custom(
        label->buffer->str,
        component->bounds,
        label->color,
        label->alpha,
        label->font
      );
    }
    else
    {
      eol_font_draw_text_justify_custom(
        label->buffer->str,
        component->bounds.x,
        component->bounds.y,
        label->color,
        label->alpha,
        label->font,
        label->justify
      );
    }
  }
}

void eol_component_draw(eolComponent *component)
{
  if (!component)return;
  if (component->data_draw == NULL)
  {
    printf("component %s has no draw function!\n",component->name);
    return;
  }
  if (!component->hidden)
  {
    component->data_draw(component);
  }
}

/*

  ***** Update *****
  
 */


eolBool eol_component_slider_update(eolComponent *component)
{
  eolComponentSlider *slider;
  eolRect sliderRect;
  eolVec2D drawPosition;
  eolInt x = 0,y = 0;
  if (!component)return eolFalse;
  slider = eol_component_get_slider_data(component);
  if (slider == NULL)return eolFalse;

  eol_mouse_get_position(&x,&y);
  
  if (slider->vertical)
  {
    drawPosition.x = component->bounds.x + (component->bounds.w * 0.5);
    drawPosition.y = component->bounds.y + (component->bounds.h * slider->position);
  }
  else
  {
    drawPosition.y = component->bounds.y + (component->bounds.h * 0.5);
    drawPosition.x = component->bounds.x + (component->bounds.w * slider->position);
  }
  
  if (slider->slider != NULL)
  {
    sliderRect.x = drawPosition.x - (slider->slider->frameWidth /2);
    sliderRect.y = drawPosition.y - (slider->slider->frameHeight /2);
    sliderRect.w = slider->slider->frameWidth;
    sliderRect.h = slider->slider->frameHeight;
  }
  else
  {
    /*TODO: support drawn sliders*/
  }
  if ((slider->sliderState == eolSliderHeld) && (eol_mouse_input_released(eolMouseLeft)))
  {
    return eolTrue;
  }
  if ((slider->sliderState == eolSliderHeld) && (eol_mouse_input_held(eolMouseLeft)))
  {
    slider->oldPosition = slider->position;
    if (slider->vertical)
    {
      if (y < component->bounds.y)y = component->bounds.y;
      if (y > component->bounds.y + component->bounds.h)y = component->bounds.y + component->bounds.h;
      y -= component->bounds.y;
      if (component->bounds.h != 0)slider->position = (float)y / component->bounds.h;
    }
    else
    {
      if (x < component->bounds.x)x = component->bounds.x;
      if (x > (component->bounds.x + component->bounds.w))x = component->bounds.x + component->bounds.w;
      x -= component->bounds.x;
      if (component->bounds.w != 0)slider->position = (float)x / component->bounds.w;
    }
    return eolFalse;
  }
  if (eol_mouse_in_rect(sliderRect))
  {
    slider->sliderState = eolSliderHigh;
    if (eol_mouse_input_pressed(eolMouseLeft))
    {
      slider->sliderState = eolSliderHeld;
    }
  }
  else
  {
    slider->sliderState = eolSliderIdle;
  }
  return eolFalse;
}

/*

  ****** Makers ******

*/

void eol_component_make_slider(
    eolComponent * component,
    eolBool        vertical,
    eolLine        sliderFile,
    eolLine        sliderHigh,
    eolLine        bar,
    eolLine        cap1,
    eolLine        cap2,
    eolVec3D       barColor,
    eolVec3D       sliderColor,
    eolFloat       startPosition,
    eolUint        sliderType
  )
{
  eolComponentSlider * slider = NULL;
  if (!component)return;
  eol_component_slider_new(component);
  slider = eol_component_get_slider_data(component);
  if (slider == NULL)
  {
    return;
  }
  
  slider->vertical = vertical;
  switch(sliderType)
  {
    case eolSliderStock:
    case eolSliderCommon:
      if (vertical)
      {
        slider->slider = _eol_component_slider[0];
        slider->sliderHigh = _eol_component_slider[1];
      }
      else
      {
        slider->slider = _eol_component_slider[2];
        slider->sliderHigh = _eol_component_slider[3];
      }
      break;
    case eolSliderCustom:
      slider->slider = eol_sprite_load(sliderFile,-1,-1);
      slider->sliderHigh = eol_sprite_load(sliderHigh,-1,-1);
      slider->bar = eol_sprite_load(bar,-1,-1);
      slider->cap1 = eol_sprite_load(cap1,-1,-1);
      slider->cap2 = eol_sprite_load(cap2,-1,-1);
      break;
  }
  slider->sliderType = sliderType;
  eol_vec3d_copy(slider->barColor,barColor);
  eol_vec3d_copy(slider->sliderColor,sliderColor);
  slider->position = slider->oldPosition = startPosition;
  
  component->data_free = eol_component_slider_free;
  component->data_draw = eol_component_slider_draw;
  component->data_update = eol_component_slider_update;
}

void eol_component_make_label(
    eolComponent * component,
    char         * text,
    eolUint        justify,
    eolInt         fontSize,
    eolBool        wordWrap,
    char         * fontName,
    eolVec3D       color,
    eolFloat       alpha
  )
{
  eolRect r;
  eolComponentLabel * label = NULL;
  if (!component)return;
  eol_component_label_new(component);
  
  label = eol_component_get_label_data(component);
  if (label == NULL)
  {
    return;
  }

  label->buffer = g_string_new(text);
  if (label->buffer == NULL)
  {
    eol_logger_message(
      EOL_LOG_ERROR,
      "eol_component:Failed to duplicate string for new label");
    eol_component_label_free(component);
    return;
  }
  
  if ((fontName != NULL)&&(strlen(fontName) > 0))
  {
    label->font = eol_font_load(fontName,fontSize);
    r = eol_font_get_bounds_custom(text,label->font);
  }
  else
  {
    r = eol_font_get_bounds(text,fontSize);
  }
  if (r.w > component->bounds.w)component->bounds.w = r.w;
  if (r.h > component->bounds.h)component->bounds.h = r.h;
  label->justify = justify;
  if (label->fontSize == -1)
  {
    /*default*/
    label->fontSize = _eol_component_label_font_size;
  }
  else
  {
    label->fontSize = fontSize;
  }
  label->alpha = alpha;
  label->wordWrap = wordWrap;
  eol_vec3d_copy(label->color,color);
  component->data_free = eol_component_label_free;
  component->data_draw = eol_component_label_draw;
  component->data_move = eol_component_label_move;
}

void eol_component_slider_new(eolComponent *component)
{
  if (component->componentData != NULL)
  {
    eol_logger_message(
      EOL_LOG_WARN,
      "eol_component_slider_new:tried to make a label out of an existing component");
    return;
  }
  component->componentData = malloc(sizeof(eolComponentSlider));
  if (component->componentData == NULL)
  {
    eol_logger_message(
      EOL_LOG_ERROR,
      "eol_component_slider_new: failed to allocate data for new slider");
    return;
  }
  memset(component->componentData,0,sizeof(eolComponentSlider));
  component->type = eolSliderComponent;
}

eolComponent *eol_component_create_label_from_config(eolKeychain *def,eolRect parentRect)
{
  eolUint       id;
  eolLine       text;
  eolLine       name;
  eolLine       justify = "LEFT";
  eolLine       wordWrap;
  eolUint       fontSize = _eol_component_label_font_size;
  eolVec3D      color;
  eolFloat      alpha = _eol_component_label_alpha;
  eolLine       fontfile; 
  eolRectFloat  rect;
  char        * font = NULL;
  
  if (!def)
  {
    eol_logger_message(EOL_LOG_WARN,"eol_component: Passed bad def parameter!");
    return NULL;
  }

  eol_vec3d_copy(color,_eol_component_label_color);
  
  eol_line_cpy(fontfile,"\0");
  eol_line_cpy(justify,"\0");
  eol_line_cpy(wordWrap,"\0");
  
  eol_keychain_get_hash_value_as_uint(&id, def, "id");
  eol_keychain_get_hash_value_as_uint(&fontSize, def, "fontSize");
  eol_keychain_get_hash_value_as_rectfloat(&rect, def, "rect");
  eol_keychain_get_hash_value_as_line(text, def, "text");
  eol_keychain_get_hash_value_as_line(name, def, "name");
  eol_keychain_get_hash_value_as_line(justify, def, "justify");
  eol_keychain_get_hash_value_as_line(fontfile, def, "fontName");
  eol_keychain_get_hash_value_as_line(wordWrap, def, "wordWrap");
  eol_keychain_get_hash_value_as_vec3d(&color, def, "color");
  eol_keychain_get_hash_value_as_float(&alpha, def, "alpha");
  
  if (strlen(fontfile) > 0)font = fontfile;
  return eol_label_new(
    id,
    name,
    rect,
    parentRect,
    eolTrue,
    text,
    eol_font_justify_from_string(justify),
    eol_true_from_string(wordWrap),
    fontSize,
    font,
    color,
    alpha
  );
}

void eol_component_label_new(eolComponent *component)
{
  if (component->componentData != NULL)
  {
    eol_logger_message(
      EOL_LOG_WARN,
      "eol_component_label_new:tried to make a label out of an existing component");
    return;
  }
  component->componentData = malloc(sizeof(eolComponentLabel));
  if (component->componentData == NULL)
  {
    eol_logger_message(
      EOL_LOG_ERROR,
      "eol_component_label_new: failed to allocate data for new label");
    return;
  }
  memset(component->componentData,0,sizeof(eolComponentLabel));
  component->type = eolLabelComponent;
}

void eol_label_get_text_size(eolComponent *comp,eolUint *w,eolUint *h)
{
  eolRect fontRect = {0,0,0,0};
  eolComponentLabel * label = NULL;
  label = eol_component_get_label_data(comp);
  if (label == NULL)return;
  if (!label->buffer)return;
  if (label->font)
  {
    fontRect = eol_font_get_bounds_custom(
      label->buffer->str,
      label->font
    );
  }
  else
  {
    fontRect = eol_font_get_bounds(
      label->buffer->str,
      label->fontSize
    );
  }
  if (w)
  {
    *w = fontRect.w;
  }
  if (h)
  {
    *h = fontRect.h;
  }
}

void eol_label_get_text(eolComponent *comp,eolLine text)
{
  eolComponentLabel * label = NULL;
  label = eol_component_get_label_data(comp);
  if (label == NULL)return;
  eol_line_cpy(text,label->buffer->str);
}

void eol_label_set_text(eolComponent *comp,char *text)
{
  eolComponentLabel * label = NULL;
  if (!text)return;
  label = eol_component_get_label_data(comp);
  if (label == NULL)return;
  label->buffer = g_string_assign(label->buffer,text);
}

eolFloat eol_slider_get_position(eolComponent *comp)
{
  eolComponentSlider * slider = NULL;
  slider = eol_component_get_slider_data(comp);
  if (!slider)return 0;
  return slider->position;
}

void eol_slider_set_position(eolComponent *comp, eolFloat newPos)
{
  eolComponentSlider * slider = NULL;
  slider = eol_component_get_slider_data(comp);
  if (!slider)return;
  if (newPos < 0)newPos = 0;
  if (newPos > 1)newPos = 1;
  slider->position = newPos;
}

/*

  ******* new components *******

 */

eolComponent *eol_slider_common_new(
    eolUint        id,
    eolLine        name,
    eolRectFloat   rect,
    eolRect        bounds,
    eolBool        vertical,
    eolVec3D       barColor,
    eolFloat       startPosition
  )
{
  if (vertical)
  {
    return eol_slider_new(id,
                          name,
                          rect,
                          bounds,
                          vertical,
                          "",
                          "",
                          "",
                          "",
                          "",
                          barColor,
                          eol_vec3d(1,1,1),
                          startPosition,
                          eolSliderCommon
                          );
  }
  else
  {
    
    return eol_slider_new(id,
                          name,
                          rect,
                          bounds,
                          vertical,
                          "",
                          "",
                          "",
                          "",
                          "",
                          barColor,
                          eol_vec3d(1,1,1),
                          startPosition,
                          eolSliderCommon
                          );
  }
}

eolComponent *eol_slider_new(
    eolUint       id,
    eolLine       name,
    eolRectFloat  rect,
    eolRect       bounds,
    eolBool       vertical,
    eolLine       slider,
    eolLine       sliderHigh,
    eolLine       bar,
    eolLine       cap1,
    eolLine       cap2,
    eolVec3D      barColor,
    eolVec3D      sliderColor,
    eolFloat      startPosition,
    eolUint       sliderType
  )
{
  eolComponent *component = NULL;
  component = eol_component_new();
  if (!component)return NULL;
  eol_component_make_slider(component,
                            vertical,
                            slider,
                            sliderHigh,
                            bar,
                            cap1,
                            cap2,
                            barColor,
                            sliderColor,
                            startPosition,
                            sliderType);
  if (component->componentData == NULL)
  {
    eol_component_free(&component);
    return NULL;
  }
  component->id = id;
  eol_line_cpy(component->name,name);
  component->type = eolSliderComponent;
  eol_rectf_copy(&component->rect,rect);
  eol_component_get_rect_from_bounds(&component->bounds,bounds, rect);
  if ((rect.w <= 1) && (rect.w >= 0))
    component->bounds.w = bounds.w * rect.w;
  else component->bounds.w = rect.w;
  if ((rect.h <= 1)  && (rect.h >= 0))
    component->bounds.h = bounds.h * rect.h;
  else component->bounds.h = rect.h;
  return component;
}

eolComponent *eol_slider_create_from_config(eolKeychain *def,eolRect parentRect)
{
  eolUint        id;
  eolLine        name;
  eolRectFloat   rect;
  eolBool        vertical = eolFalse;
  eolVec3D       barColor;
  eolFloat       startPosition = 0;
  eolLine        sliderType;
  eolInt         sliderStyle = eolSliderStock;
  eolLine        slider = "";
  eolLine        sliderHigh = "";
  eolLine        bar = "";
  eolLine        cap1 = "";
  eolLine        cap2 = "";
  
  if (!def)
  {
    eol_logger_message(EOL_LOG_WARN,"eol_component: passed bad config parameter");
    return NULL;
  }
  eol_keychain_get_hash_value_as_line(name, def, "name");
  eol_keychain_get_hash_value_as_uint(&id, def, "id");
  eol_keychain_get_hash_value_as_rectfloat(&rect, def, "rect");
  eol_keychain_get_hash_value_as_bool(&vertical, def, "vertical");
  eol_keychain_get_hash_value_as_vec3d(&barColor, def, "barColor");
  eol_keychain_get_hash_value_as_line(sliderType, def, "sliderType");
  eol_keychain_get_hash_value_as_line(slider, def, "slider");
  eol_keychain_get_hash_value_as_line(sliderHigh, def, "sliderHigh");
  eol_keychain_get_hash_value_as_line(bar, def, "bar");
  eol_keychain_get_hash_value_as_line(cap1, def, "cap1");
  eol_keychain_get_hash_value_as_line(cap2, def, "cap2");
  eol_keychain_get_hash_value_as_float(&startPosition, def, "startPosition");

  if (eol_line_cmp(sliderType,"COMMON") == 0)
  {
    sliderStyle = eolSliderCommon;
  }
  return eol_slider_new(id,
                        name,
                        rect,
                        parentRect,
                        vertical,
                        slider,
                        sliderHigh,
                        bar,
                        cap1,
                        cap2,
                        barColor,
                        eol_vec3d(1,1,1),
                        startPosition,
                        sliderStyle
                      );
}


eolComponent *eol_label_new_default(
  eolUint        id,
  eolLine        name,
  eolRectFloat   rect,
  eolRect        bounds,
  char         * text,
  eolBool        wordWrap
)
{
  return eol_label_new(
    id,
    name,
    rect,
    bounds,
    eolFalse,
    text,
    eolJustifyLeft,
    wordWrap,
    _eol_component_label_font_size,
    NULL,
    _eol_component_label_color,
    _eol_component_label_alpha
  );
}

eolComponent *eol_label_new(
    eolUint        id,
    eolLine        name,
    eolRectFloat   rect,
    eolRect        bounds,
    eolBool        canHasFocus,
    char         * text,
    eolUint        justify,
    eolBool        wordWrap,
    eolInt         fontSize,
    char         * fontName,
    eolVec3D       color,
    eolFloat       alpha
  )
{
  eolComponent *component = NULL;
  if (!text)return NULL;
  component = eol_component_new();
  if (!component)return NULL;
  eol_component_get_rect_from_bounds(&component->bounds,bounds, rect);
  eol_component_make_label(
    component,
    text,
    justify,
    fontSize,
    wordWrap,
    fontName,
    color,
    alpha
  );
  if (component->componentData == NULL)
  {
    eol_component_free(&component);
    return NULL;
  }
  component->id = id;
  strncpy(component->name,name,EOLWORDLEN);
  eol_rectf_copy(&component->rect,rect);
  component->type = eolLabelComponent;
  return component;
}

void eol_component_label_move(eolComponent *component,eolRect newbounds)
{
  if (!component)return;
  eol_rect_copy(&component->canvas,newbounds);
  eol_component_get_rect_from_bounds(&component->bounds,newbounds,component->rect);
}

/*utility functions*/
void eol_component_get_rect_from_bounds(eolRect *rect,eolRect canvas, eolRectFloat bounds)
{
  eolRectFloat rbounds;
  if (!rect)return;

  if (bounds.x < -1)
  {
    /*raw number from right edge*/
    rbounds.x = (canvas.w + bounds.x) / (float)canvas.w;
  }
  else if (bounds.x < 0)
  {
    /*percent of canvas from right*/
    rbounds.x = 1 + bounds.x;
  }
  else if (bounds.x <= 1)
  {
    /*percent of canvas from left*/
    rbounds.x = bounds.x;
  }
  else
  {
    rbounds.x = bounds.x / (float)canvas.w;
  }

  if (bounds.y < -1)
  {
    /*raw number from right edge*/
    rbounds.y = (canvas.h + bounds.y) / (float)canvas.h;
  }
  else if (bounds.y < 0)
  {
    /*percent of canvas from right*/
    rbounds.y = 1 + bounds.y;
  }
  else if (bounds.y <= 1)
  {
    /*percent of canvas from left*/
    rbounds.y = bounds.y;
  }
  else
  {
    rbounds.y = bounds.y / (float)canvas.h;
  }


  if (bounds.w <= 0)
  {
    /*fill to end*/
    rbounds.w = 1 - rbounds.x;
  }
  else if (bounds.w > 1)
  {
    /*exact size*/
    rbounds.w = bounds.w / (float)canvas.w;
  }
  else
  {
    /*% of canvas*/
    rbounds.w = bounds.w;
  }

  if (bounds.h <= 0)
  {
    /*fill to end*/
    rbounds.h = 1 - rbounds.y;
  }
  else if (bounds.h > 1)
  {
    /*exact size*/
    rbounds.h = bounds.h / (float)canvas.h;
  }
  else
  {
    /*% of canvas*/
    rbounds.h = bounds.h;
  }
  
  rect->x = canvas.x + (rbounds.x * canvas.w);
  rect->y = canvas.y + (rbounds.y * canvas.h);
  rect->w = canvas.w * rbounds.w;
  rect->h = canvas.h * rbounds.h;
}

eolComponent * eol_component_make_from_config(eolKeychain *config,eolRect boundingRect)
{
  eolLine typecheck = "";
  eolComponent *comp = NULL;
  if (!eol_keychain_get_hash_value_as_line(typecheck, config, "type"))
  {
    eol_logger_message(EOL_LOG_WARN,"eol_component: Config missing component data");
    return NULL;
  }
  if (eol_line_cmp(typecheck,"BUTTON") == 0)
  {
    comp = eol_component_button_load(boundingRect,config);
  }
  else if (eol_line_cmp(typecheck,"LABEL") == 0)
  {
    comp = eol_component_create_label_from_config(config,boundingRect);
  }
  else if (eol_line_cmp(typecheck,"SLIDER") == 0)
  {
    comp = eol_slider_create_from_config(config,boundingRect);
  }
  else if (eol_line_cmp(typecheck,"PERCENT") == 0)
  {
    comp = eol_percent_bar_create_from_config(config,boundingRect);
  }
  else if (eol_line_cmp(typecheck,"LIST") == 0)
  {
    comp = eol_list_create_from_config(boundingRect,config);
  }
  else if (eol_line_cmp(typecheck,"ENTRY") == 0)
  {
    comp = eol_entry_create_from_config(config,boundingRect);
  }
  else if (eol_line_cmp(typecheck,"ACTOR") == 0)
  {
    comp = eol_component_actor_create_from_config(config,boundingRect);
  }
  else if (eol_line_cmp(typecheck,"CHECK") == 0)
  {
    comp = eol_check_create_from_config(config,boundingRect);
  }
  else eol_logger_message(EOL_LOG_WARN,"eol_component: Config component type unsupported");
  if (!comp)return NULL;
  /*common to all components config*/
  eol_keychain_get_hash_value_as_bool(&comp->canHasFocus, config, "canHaveFocus");
  eol_keychain_get_hash_value_as_bool(&comp->hasFocus, config, "focusDefault");
  eol_keychain_get_hash_value_as_bool(&comp->hidden, config, "hidden");
  return comp;
}

/*eol@eof*/

