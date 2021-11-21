#pragma once

#include "../system/common/ui_event_destination.hpp"

#include "../system/common/font.hpp"


namespace plate {

class widget_keyboard : public ui_event_destination
{

public:

  void init(const std::shared_ptr<state>& _ui, float scale) noexcept
  {
    ui_event_destination::init(_ui, {{0,0}, {_ui->pixel_width_, static_cast<int>(270 * _ui->pixel_ratio_)}},
                                                  Prop::Input | Prop::Swipe | Prop::Display),

    scale_ = scale;
    mode_   = 0;

    characters_ = uk_keyboard;

    upload_vertex();

    char_uniform_buffer_.usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    wbox_ = ui_event_destination::make_ui<widget_box>(ui_, coords_, Prop::None, shared_from_this(),
                                                                                      ui_->bg_color_);

    anim_ = my_height();

    upload_uniform();

    std::vector<anim::targets> t{{{&anim_, my_height(), 0}}};

    anim_handle_ = ui_->anim_.add(std::move(t), anim::SoftSoft, 0.15, [this]
    {
      upload_uniform();
    },
    [this]
    {
      anim_handle_ = 0;
    });
  };


  void set_click_cb(std::function< void (std::uint32_t cmd, const std::string& c)> cb)
  {
    click_cb_ = cb;
  };

  
  void display() noexcept
  {
    wbox_->display();

    ui_->shader_text_msdf_->draw(ui_->projection_, ui_->alpha_, char_uniform_buffer_, char_vertex_buffer_,
         ui_->font_->texture_, ui_->txt_color_[ui_->color_mode_],
         ui_->font_->get_atlas_width(), ui_->font_->get_atlas_height());

    if (key_anim_)
    {
      wbox_selected_->display();
      auto c = ui_->txt_color_[ui_->color_mode_];
      c.a = key_anim_/255.0f;
      ui_->shader_text_msdf_->draw(ui_->projection_, ui_->alpha_, char_uniform_buffer_, key_vertex_buffer_,
                  ui_->font_->texture_, c, ui_->font_->get_atlas_width(), ui_->font_->get_atlas_height());
    }

    if (bs_repeat_ > 0)
    {
      if (!bs_skipped_)
        bs_skipped_ = true;
      else
      {
        bs_repeat_ -= ui_->frame_diff_;
        if (bs_repeat_ <= 0)
        {
          bs_repeat_ = 0.1;
          bs_deleted_ = true;

          ui_->add_to_command_queue([this]
          {
            ui_event::js_vibrate(1);
            ui_->incoming_key_event(ui_event::KeyEventPress, "", "Backspace", ui_event::KeyModNone);
          });
        }
      }

      ui_->do_draw();
    }
  };


  void open()
  {
    if (!opening_ && anim_handle_) // must be closing
    {
      opening_ = true;

      std::vector<anim::targets> t{{{&anim_, anim_, 0}}};

      ui_->anim_.erase(anim_handle_);

      anim_handle_ = ui_->anim_.add(std::move(t), anim::SoftSoft, 0.15, [this, self{shared_from_this()}]
      {
        upload_uniform();
      },
      [this, self{shared_from_this()}]
      {
        anim_handle_ = 0;
      });
    }
  };


  void close(std::function< void ()> cb)
  {
    if (!opening_ && anim_handle_) // must already be closing
      return;

    opening_ = false;

    ui_->incoming_virtual_keyboard_quit();

    std::vector<anim::targets> t{{{&anim_, 0, my_height()}}};

    anim_handle_ = ui_->anim_.add(std::move(t), anim::SoftSoft, 0.25, [this, self{shared_from_this()}]
    {
      upload_uniform();
    },
    [this, self{shared_from_this()}, cb]
    {
      cb();
    });
  };


  void cancel_delete()
  {
    bs_repeat_ = 0;
  };


  void ui_out()
  {
  };



  bool ui_mouse_position_update()
  {
    auto& m = ui_->mouse_metric_;

    return true;
  };


  void ui_mouse_button_update()
  {
    auto& m = ui_->mouse_metric_;

    if (m.click)
    {
      // what was pressed?

      auto& keys = characters_[mode_];
      
      auto row_height = my_height() / 3;
      
      float button_width = my_width() / 10.0;

      float x[3] = { 0.0f, button_width / 2.0f, (button_width * 3) / 2.0f };

      int row =  2 - (m.pos.y / row_height);
      int col = (m.pos.x - x[row]) / button_width;

      if (m.pos.x >= x[row] && col < keys[row].size())
      {
        ui_->incoming_key_event(ui_event::KeyEventPress, keys[row][col], 0, ui_event::KeyModNone);
        return;
      }
    }
  };


  bool ui_touch_update(int id)
  {
    auto& m = ui_->touch_metric_[id];

    log_debug(FMT_COMPILE("id: {} st: {}"), id, m.st);

    if (m.st == ui_event::TouchEventCancel)
    {
      bs_repeat_ = 0;
      bs_deleted_ = true;
    }

    if (m.st == ui_event::TouchEventDown)
    {
      //auto& keys = uk_keyboard[mode_];
      auto& keys = characters_[mode_];
  
      auto row_height = my_height() / 4;
  
      float button_width = my_width() / 10.0;
  
      float x[4] = { 0.0f, button_width / 2.0f, (button_width * 3) / 2.0f, (button_width * 6) / 2.0f };
  
      int row =  (3 - (m.pos.y / row_height));
      int col = (m.pos.x - x[row]) / button_width;
  
      if ((row <= 2) && (m.pos.x >= x[row]) && (col < keys[row].size()))
      {
        create_impression(keys[row][col], m.pos.x, m.pos.y);

        return true;
      }

      if ((row == 2) && (m.pos.x >= my_width() - x[row])) // delete
      {
        bs_deleted_ = false;
        bs_repeat_ = 0.1;
        bs_skipped_ = false;

        return true;
      }

      if ((row == 3) && (m.pos.x >= x[row]) && (m.pos.x <= (my_width() - x[row]))) // space
      {
        create_impression("␣", m.pos.x, m.pos.y);
        return true;
      }

      return true;
    }

    if (m.st == ui_event::TouchEventUp)
    {
      if (!bs_deleted_)
      {
        ui_event::js_vibrate(2);
        ui_->incoming_key_event(ui_event::KeyEventPress, "", "Backspace", ui_event::KeyModNone);
        bs_deleted_ = true;
      }

      bs_repeat_ = 0;

      if (m.swipe)
      {
        return true;
      }

      auto& keys = characters_[mode_];
         
      auto row_height = my_height() / 4;
        
      float button_width = my_width() / 10.0;
      
      float x[4] = { 0.0f, button_width / 2.0f, (button_width * 3) / 2.0f, (button_width * 6) / 2.0f };
  
      int row =  (3 - (m.pos.y / row_height));
      int col = (m.pos.x - x[row]) / button_width;
  
      if ((row <= 2) && (m.pos.x >= x[row]) && (col < keys[row].size()))
      {
        ui_event::js_vibrate(2);
        ui_->incoming_key_event(ui_event::KeyEventPress, keys[row][col], "", ui_event::KeyModNone);

        std::vector<anim::targets> t{{{&key_anim_, 255, 0}}};

        ui_->anim_.add(std::move(t), anim::HoldSoftSoft, 0.18, [this, self{shared_from_this()}]
        {
          wbox_selected_->update_color_alpha(key_anim_/255.0f);
        });

        if (shift_auto_off_)
        {
          shift_auto_off_ = false;
          mode_ = 0;
          upload_vertex();
        }

        return true;
      }

      key_anim_ = 0;

      if ((row == 2) && (m.pos.x >= my_width() - x[row])) // delete
      {
        return true;
      }

      if ((row == 2) && (m.pos.x <= x[row])) // shift
      {
        if (m.double_click)
        {
          ui_event::js_vibrate(4);

          mode_ = 1;
          shift_auto_off_ = false;

          upload_vertex();
        }
        else
        {
          ui_event::js_vibrate(2);

          if (mode_ == 0)
          {
            mode_ = 1;
            shift_auto_off_ = true;
          }
          else
          {
            mode_ = 0;
            shift_auto_off_ = false;
          }

          upload_vertex();
        }
        return true;
      }

      if ((row == 3) && (m.pos.x < button_width * 1.5)) // hash
      {
        if (mode_ <= 1)
          mode_ = 2;
        else
          if (mode_ == 2)
            mode_ = 3;
          else
            mode_ = 0;

        ui_event::js_vibrate(2);

        shift_auto_off_ = false;
        upload_vertex();

        return true;
      }

      if ((row == 3) && (m.pos.x < button_width * 3)) // lang
      {
        // todo

        return true;
      }

      if ((row == 3) && (m.pos.x <= (my_width() - x[row]))) // space
      {
        ui_event::js_vibrate(2);
        ui_->incoming_key_event(ui_event::KeyEventPress, " ", "", ui_event::KeyModNone);

        std::vector<anim::targets> t{{{&key_anim_, 255, 0}}};

        ui_->anim_.add(std::move(t), anim::HoldSoftSoft, 0.18, [this, self{shared_from_this()}]
        {
          wbox_selected_->update_color_alpha(key_anim_/255.0f);
        });

        if (shift_auto_off_)
        {
          shift_auto_off_ = false;
          mode_ = 0;
          upload_vertex();
        }

        return true;
      }

      if ((row == 3) && (m.pos.x > my_width() - x[row])) // return
      {
        ui_event::js_vibrate(4);
        ui_->incoming_key_event(ui_event::KeyEventPress, "", "Go", ui_event::KeyModNone);

        return true;
      }
    }

    if (m.st == ui_event::TouchEventMove)
    {
      if (m.swipe)
      {
        key_anim_ = 0;
      }
      return true;
    }

    return true;
  };


  std::string_view name() const noexcept
  {
    return "keyboard";
  }

private:


  // create a key impression

  void create_impression(const std::string& s, int x, int y)
  {
    auto row_height = my_height() / 4;
    float button_width = my_width() / 10.0;

    wbox_selected_->update_color_alpha(1.0f);
    wbox_selected_->set_offset({static_cast<int>(x - button_width / 2), static_cast<int>(y + row_height / 2)});
    key_anim_ = 255;
  
    std::vector<float> vertex;
  
    float font_scale = ui_->pixel_ratio_ * ui_->font_size_ * scale_;
  
    auto vpos = ui_->font_->generate_vertex(s, vertex, font_scale * 1.5);
  
    int x_shift = (x - button_width / 2) + (button_width - vpos.p2.x) / 2;
    int y_shift = (y + row_height / 2)   + (button_width - vpos.p2.y) / 2;
  
    int vertex_pos = 0;
  
    for (int o = 0; o < 6; ++o)
    {
      vertex[vertex_pos] += x_shift;
      vertex_pos += 1;
      vertex[vertex_pos] += y_shift;
      vertex_pos += 5;
    }
 
    key_vertex_buffer_.upload(reinterpret_cast<std::byte*>(vertex.data()), vertex.size() / 6, 6 * sizeof(float));
  };

    
  void upload_vertex()
  {
    float font_scale = ui_->pixel_ratio_ * ui_->font_size_ * scale_;

    vertex_.clear();

    // build each character and put in corret position

    int vertex_pos = 0;

    //auto& keys = uk_keyboard[mode_];
    auto& keys = characters_[mode_];

    auto row_height = my_height() / 4;

    float button_width = my_width() / 10.0;
    int button_width_i = static_cast<int>(button_width);

    auto max_height = ui_->font_->get_max_height() * font_scale;

    float y_space = (row_height - ui_->font_->get_max_height()) / 2;

    if (!wbox_selected_)
    {
      auto c = ui_->bg_color_; // allow it to be see through..
      c[0].a = c[1].a = 1.0;

      gpu::int_box coords;
      coords.p1 = {0, 0};
      coords.p2 = {button_width_i, button_width_i};

      wbox_selected_ = ui_event_destination::make_ui<widget_box>(ui_, coords, Prop::None, shared_from_this(), c);
    }

    float x[4] = { 0.0f, button_width / 2.0f, (button_width * 3) / 2.0f, (button_width * 6) / 2.0f };

    for (std::size_t i = 0; i < 3; ++i)
    {
      auto num_columns = keys[i].size();

      auto col_width  = x[i] / num_columns;
      auto col_offset = (my_width() - x[i]) / 2;

      for (std::size_t j = 0; j < num_columns; ++j)
      {
        auto vpos = ui_->font_->generate_vertex(keys[i][j], vertex_, font_scale);

        // shift it to the correct position

        int x_shift = x[i] + (button_width * j) + ((button_width - vpos.p2.x) / 2);
        int y_shift = (row_height * (3 - i)) + y_space;

        for (int o = 0; o < 6; ++o)
        {
          vertex_[vertex_pos] += x_shift;
          vertex_pos += 1;
          vertex_[vertex_pos] += y_shift;
          vertex_pos += 5;
        }
      }
    }

    // add in 'specials'

    { //shift, 3rd row left
      auto vpos = ui_->font_->generate_vertex("^", vertex_, font_scale);

      int x_shift = ((x[2] - vpos.p2.x) / 2);
      int y_shift = (row_height * 1) + y_space;

      for (int o = 0; o < 6; ++o)
      {
        vertex_[vertex_pos] += x_shift;
        vertex_pos += 1;
        vertex_[vertex_pos] += y_shift;
        vertex_pos += 5;
      }

      if (mode_ == 1 && !shift_auto_off_) // per shift
      {
        auto vpos = ui_->font_->generate_vertex("⸋", vertex_, font_scale);
  
        int x_shift = ((x[2] - vpos.p2.x) / 2);
        int y_shift = (row_height * 1) + y_space - (font_scale * 20);
  
        for (int o = 0; o < 6; ++o)
        {
          vertex_[vertex_pos] += x_shift;
          vertex_pos += 1;
          vertex_[vertex_pos] += y_shift;
          vertex_pos += 5;
        }
      }
    }
    { // Backspace, 3rd row from top far right
      auto vpos = ui_->font_->generate_vertex("⟨", vertex_, font_scale);
      //auto vpos = ui_->font_->generate_vertex("ꭗ", vertex_, font_scale);

      int x_shift = (my_width() - x[2]) + ((x[2] - vpos.p2.x) / 2);
      int y_shift = (row_height * 1) + y_space;

      for (int o = 0; o < 6; ++o)
      {
        vertex_[vertex_pos] += x_shift;
        vertex_pos += 1;
        vertex_[vertex_pos] += y_shift;
        vertex_pos += 5;
      }
    }
    { // #, bottom row left
      auto vpos = ui_->font_->generate_vertex("#", vertex_, font_scale);

      int x_shift = (((button_width * 1.5) - vpos.p2.x) / 2);
      int y_shift = (row_height - (vpos.p2.y - vpos.p1.y)) / 2 - vpos.p1.y;

      for (int o = 0; o < 6; ++o)
      {
        vertex_[vertex_pos] += x_shift;
        vertex_pos += 1;
        vertex_[vertex_pos] += y_shift;
        vertex_pos += 5;
      }

      if (mode_ >= 2)
      {
        auto vpos = ui_->font_->generate_vertex(mode_ == 2 ? "₁" : "₂", vertex_, font_scale);

        int x_shift = (((button_width * 1.5) - vpos.p2.x) / 2) + (font_scale * 20);
        int y_shift = (row_height - (vpos.p2.y - vpos.p1.y)) / 2 - vpos.p1.y;

        for (int o = 0; o < 6; ++o)
        {
          vertex_[vertex_pos] += x_shift;
          vertex_pos += 1;
          vertex_[vertex_pos] += y_shift;
          vertex_pos += 5;
        }
      }
    }
    { // lang, bottom row 2nd left
      std::string t; // as a string as it confuses vim otherwise
      t.push_back(static_cast<char>(210));
      t.push_back(static_cast<char>(137));

      auto vpos = ui_->font_->generate_vertex(t, vertex_, font_scale);

      int x_shift = (button_width * 1.5) + ((button_width * 1.5) - vpos.p2.x)/2;
      int y_shift = (row_height - (vpos.p2.y - vpos.p1.y)) / 2 - vpos.p1.y;

      for (int o = 0; o < 6; ++o)
      {
        vertex_[vertex_pos] += x_shift;
        vertex_pos += 1;
        vertex_[vertex_pos] += y_shift;
        vertex_pos += 5;
      }
    }
    { // Space, 4th/bottom row
      auto vpos = ui_->font_->generate_vertex("␣", vertex_, font_scale);

      // stretch..

      vertex_[vertex_pos + 6] = my_width() - x[3]*2;// - button_width;
      vertex_[vertex_pos + 18] = my_width() - x[3]*2;// - button_width;
      vertex_[vertex_pos + 30] = my_width() - x[3]*2;// - button_width;

      vpos.p2.x = my_width() - x[3]*2;// - button_width;

      int x_shift = ((my_width() - vpos.p2.x) / 2);
      int y_shift = (row_height - (vpos.p2.y - vpos.p1.y)) / 2 - vpos.p1.y;

      for (int o = 0; o < 6; ++o)
      {
        vertex_[vertex_pos] += x_shift;
        vertex_pos += 1;
        vertex_[vertex_pos] += y_shift;
        vertex_pos += 5;
      }
    }
    { // return, bottom row right
      auto vpos = ui_->font_->generate_vertex("‣", vertex_, font_scale * 1.3);

      int x_shift = (my_width() - x[3]) + ((x[3] - vpos.p2.x) / 2);
      int y_shift = (row_height - (vpos.p2.y - vpos.p1.y)) / 2 - vpos.p1.y;

      for (int o = 0; o < 6; ++o)
      {
        vertex_[vertex_pos] += x_shift;
        vertex_pos += 1;
        vertex_[vertex_pos] += y_shift;
        vertex_pos += 5;
      }
    }


    char_vertex_buffer_.upload(reinterpret_cast<std::byte*>(vertex_.data()), vertex_.size() / 6, 6 * sizeof(float));
  };


  void upload_uniform()
  {
    auto ptr = char_uniform_buffer_.allocate_staging(1, sizeof(shader_text::ubo));
  
    shader_text::ubo* u = new (ptr) shader_text::ubo;
  
    u->offset = { { static_cast<float>(coords_.p1.x) }, { static_cast<float>(coords_.p1.y) - anim_ }, { 0.0f }, { 0.0f   } };
    u->rot    = { { 0.0f }, { 0.0f }, { 0.0f }, { 0.0f } };
    u->scale  = { system_scale_, system_scale_ };
  
    char_uniform_buffer_.upload();

    if (wbox_)
      wbox_->set_offset({0, -anim_});
  };


  std::vector<std::string> languages = { "English", "Ελληνικά" };

  std::vector<std::vector<std::string>> uk_keyboard[4] = { { { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
                                                              { "a", "s", "d", "f", "g", "h", "j", "k", "l" },
                                                                { "z", "x", "c", "v", "b", "n", "m" }},
                                                           { { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
                                                              { "A", "S", "D", "F", "G", "H", "J", "K", "L" },
                                                                { "Z", "X", "C", "V", "B", "N", "M" }},
                                                           { { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
                                                              { "-", "/", ":", ";", "(", ")", "£", "&", "@" },
                                                                { ".", ",", "?", "!", "'", "\"", "+" }},
                                                           { { "[", "]", "{", "}", "#", "%", "^", "*", "+", "="},
                                                              { "_", "•", "|", "~", "<", ">", "€", "$", "¥" },
                                                                { ".", ",", "?", "!", "'", "\"", "+" }} };

  std::vector<std::vector<std::string>> gr_keyboard[4] = { { { ";", "ς", "ε", "ρ", "τ", "υ", "θ", "ι", "ο", "π"},
                                                              { "α", "σ", "δ", "φ", "γ", "η", "ξ", "κ", "λ"},
                                                                { "ζ", "χ", "ψ", "ω", "β", "ν", "μ"}},
                                                           { { ":", "ς", "Ε", "Ρ", "Τ", "Υ", "Θ", "Ι", "Ο", "Π"},
                                                              { "Α", "Σ", "Δ", "Φ", "Γ", "Η", "Ξ", "Κ", "Λ"},
                                                                { "Ζ", "Χ", "Ψ", "Ω", "Β", "Ν", "Μ"}},
                                                           { { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
                                                              { "-", "/", ":", ";", "(", ")", "£", "&", "@" },
                                                                { ".", ",", "?", "!", "'", "\"", "+" }},
                                                           { { "[", "]", "{", "}", "#", "%", "^", "*", "+", "="},
                                                              { "_", "•", "|", "~", "<", ">", "€", "$", "¥" },
                                                                { ".", ",", "?", "!", "'", "\"", "+" }} };


  std::vector<float> vertex_;

  buffer char_vertex_buffer_;
  buffer char_uniform_buffer_;

  int anim_{0};
  bool opening_{true};

  int mode_{0}; // 0= lower, 1 = upper, 2 = symbols

  int anim_handle_{0};

  std::vector<std::vector<std::string>>* characters_;

  std::function<void (std::uint32_t cmd, const std::string& c)> click_cb_;

  float scale_; // font
  float system_scale_{1}; // for when shifting/shrinking keyboard

  int key_anim_{0};
  buffer key_vertex_buffer_;

  float bs_repeat_{0};
  bool bs_skipped_{false}; // hack - fixme as first update time may be wrong as display paused
  bool bs_deleted_{true};  // whether a delete has been issued since the delete button was pressed

  bool shift_auto_off_{false};

  std::shared_ptr<widget_box> wbox_;
  std::shared_ptr<widget_box> wbox_selected_;

}; // widget_keyboard


namespace keyboard {

  std::shared_ptr<widget_keyboard> widget_keyboard_;

  bool inline is_open() noexcept
  {
    return widget_keyboard_ ? true : false;
  }


  void open(std::shared_ptr<state>& s) noexcept
  {
    if (widget_keyboard_)
      widget_keyboard_->open(); // cancels any pending close
    else
      widget_keyboard_ = ui_event_destination::make_ui<widget_keyboard>(s, 0.8);
  }


  void close() noexcept
  {
    if (widget_keyboard_)
    {
      widget_keyboard_->close([]
      {
        widget_keyboard_->disconnect_from_parent();
        widget_keyboard_.reset();
      });
    }
  }


  void cancel_delete() noexcept
  {
    if (widget_keyboard_)
      widget_keyboard_->cancel_delete();
  }


}; // namespace keyboard


} // namespace plate

