#include <flashback/signup_page.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <types.pb.h>

using namespace flashback;

signup_page::signup_page(std::shared_ptr<client> client)
    : m_client{client}
{
    auto [component, content] = prepare_components();
    page::display(component, content);
}

std::pair<ftxui::Component, std::function<ftxui::Element()>> signup_page::prepare_components()
{
    ftxui::InputOption name_traits{};
    name_traits.on_enter = std::bind(&signup_page::verify_submit, this);
    ftxui::Component name{ftxui::Input(&m_name, name_traits)};

    ftxui::InputOption email_traits{};
    email_traits.on_enter = std::bind(&signup_page::verify_submit, this);
    ftxui::Component email_field{ftxui::Input(&m_email, email_traits)};

    ftxui::InputOption password_traits{};
    password_traits.password = true;
    password_traits.on_enter = std::bind(&signup_page::verify_submit, this);
    ftxui::Component password{ftxui::Input(&m_password, password_traits)};
    ftxui::Component verify_password{ftxui::Input(&m_verify_password, password_traits)};

    ftxui::Element unmatched_password{ftxui::text("password does not match") | ftxui::color(ftxui::Color::Red) | ftxui::bold};
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
        ftxui::Button("Create Account", [this] {
            verify_submit();
        }, button_style)
    };

    ftxui::Component component{ftxui::Container::Vertical({name, email_field, password, verify_password, submit_button})};

    std::function<ftxui::Element()> content{
        [name, email_field, password, verify_password, submit_button] {
            return ftxui::vbox(ftxui::vbox(ftxui::hbox(ftxui::text("Full Name: "), name->Render()),
                                           ftxui::hbox(ftxui::text("Email: "), email_field->Render()),
                                           ftxui::hbox(ftxui::text("Password: "), password->Render()),
                                           ftxui::hbox(ftxui::text("Verify Password: "), verify_password->Render()),
                                           submit_button->Render()) | ftxui::borderEmpty, ftxui::borderEmpty);
        }
    };

    return {component, content};
}

bool signup_page::verify() const
{
    return !m_name.empty() && !m_email.empty() && !m_password.empty() && m_password == m_verify_password;
}

void signup_page::submit()
{
    try
    {
        if (m_client->signup(m_name, m_email, m_password))
        {
            page::close();
        }
        else
        {
        }
    }
    catch (std::exception const& exp)
    {
        std::cerr << exp.what() << std::endl;
        page::close();
    }
}

void signup_page::verify_submit()
{
    if (verify())
    {
        submit();
    }
    else
    {
    }
}
