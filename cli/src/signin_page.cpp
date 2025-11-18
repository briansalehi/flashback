#include <functional>
#include <flashback/signin_page.hpp>
#include <ftxui/component/component.hpp>

using namespace flashback;

signin_page::signin_page(std::shared_ptr<client> client)
    : m_client{client}
{
    ftxui::InputOption email_traits{};
    email_traits.on_enter = std::bind(&signin_page::verify_submit, this);
    ftxui::Component email_field{ftxui::Input(&m_email, email_traits)};
    email_field = ftxui::CatchEvent(email_field, [](ftxui::Event event) { return event.is_character(); });

    ftxui::InputOption password_traits{};
    password_traits.password = true;
    password_traits.on_enter = std::bind(&signin_page::verify_submit, this);
    ftxui::Component password{ftxui::Input(&m_password, password_traits)};
    password = ftxui::CatchEvent(password, [](ftxui::Event event) { return event.is_character(); });

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
        ftxui::Button("Sign In", [this] {
            verify_submit();
        }, button_style)
    };

    ftxui::Component component{
        ftxui::Container::Vertical({
            email_field,
            password,
            submit_button
        })
    };

    std::function<ftxui::Element()> content{
        [email_field, password, submit_button] {
            return ftxui::vbox(
                ftxui::vbox(
                    ftxui::hbox(ftxui::text("Email: "), email_field->Render()),
                    ftxui::hbox(ftxui::text("Password: "), password->Render()),
                    submit_button->Render()
                ) | ftxui::borderEmpty,
                ftxui::borderEmpty
            );
        }
    };

    page::display(component, content);
}

bool signin_page::verify()
{
    return !m_email.empty() &&
        !m_password.empty();
}

void signin_page::submit()
{
    try
    {
        auto user{std::make_unique<User>()};
        user->set_email(m_email);
        user->set_password(m_password);
        m_client->user(std::move(user));
        std::shared_ptr<SignInResponse> response{m_client->signin()};
        m_client->user(std::make_unique<User>(response->user()));

        if (response->success())
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

void signin_page::verify_submit()
{
    if (verify())
    {
        submit();
    }
    else
    {
    }
}
