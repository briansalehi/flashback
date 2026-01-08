#include <functional>
#include <flashback/reset_password_page.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <types.pb.h>

using namespace flashback;

reset_password_page::reset_password_page(std::shared_ptr<client> client)
    : m_client{client}
{
    auto [component, content] = reset_password_page::prepare_components();
    page::display(component, content);
}

std::pair<ftxui::Component, std::function<ftxui::Element()>> reset_password_page::prepare_components()
{
    ftxui::InputOption email_traits{};
    email_traits.on_enter = std::bind(&reset_password_page::verify_submit, this);
    ftxui::Component email_field{ftxui::Input(&m_email, email_traits)};

    ftxui::InputOption password_traits{};
    password_traits.password = true;
    password_traits.on_enter = std::bind(&reset_password_page::verify_submit, this);
    ftxui::Component password{ftxui::Input(&m_password, password_traits)};
    ftxui::Component verify_password{ftxui::Input(&m_verify_password, password_traits)};

    ftxui::Element unmatched_password{
        ftxui::text("password does not match") | ftxui::color(ftxui::Color::Red) | ftxui::bold
    };
    // unmatched_password = ftxui::Maybe(unmatched_password, [&] -> bool { return password != verify_password; });

    ftxui::ButtonOption button_style{ftxui::ButtonOption::Animated()};
    button_style.transform = [](const ftxui::EntryState& s) {
        auto element = ftxui::text(s.label);
        if (s.focused)
        {
            element |= ftxui::bold;
        }
        return element | ftxui::center | ftxui::borderEmpty | ftxui::flex;
    };

    ftxui::Component submit_button{
        ftxui::Button("Reset Password", [this] {
            verify_submit();
        }, button_style)
    };

    ftxui::Component component{
        ftxui::Container::Vertical({
            email_field,
            password,
            verify_password,
            submit_button
        })
    };

    std::function<ftxui::Element()> content{
        [email_field, password, verify_password, submit_button] {
            return ftxui::vbox(
                ftxui::vbox(
                    ftxui::hbox(ftxui::text("Email: "), email_field->Render()),
                    ftxui::hbox(ftxui::text("Password: "), password->Render()),
                    ftxui::hbox(ftxui::text("Verify Password: "), verify_password->Render()),
                    submit_button->Render()
                ) | ftxui::borderEmpty,
                ftxui::borderEmpty
            );
        }
    };

    return {component, content};
}

bool reset_password_page::verify()
{
    return !m_email.empty() &&
        !m_password.empty() &&
        m_password == m_verify_password;
}

void reset_password_page::submit()
{
    try
    {
    }
    catch (std::exception const& exp)
    {
        std::cerr << exp.what() << std::endl;
        page::close();
    }
}

void reset_password_page::verify_submit()
{
    if (verify())
    {
        submit();
    }
    else
    {
    }
}
