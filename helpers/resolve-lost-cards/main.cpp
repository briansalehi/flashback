#include <string>
#include <vector>
#include <cstdint>
#include <print>
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <pqxx/pqxx>
#include <Card.hpp>

constexpr auto lost_cards_query{"select * from get_lost_cards()"};
constexpr auto duplicate_card_query{"select * from get_duplicate_card($1)"};
constexpr auto get_blocks_query{"select * from get_blocks($1)"};

pqxx::connection connection{"dbname=flashback user=flashback host=localhost port=5432"};

Card get_lost_card(pqxx::row const& row)
{
    Card card;
    card.subject_id = row["sid"].as<uint64_t>();
    card.card_id = row["card"].as<uint64_t>();
    card.topic_position = row["tid"].as<uint32_t>();
    card.card_position = row["position"].as<uint32_t>();
    card.subject = row["subject"].as<std::string>();
    card.topic = row["topic"].as<std::string>();
    card.level = row["level"].as<std::string>();
    card.state = row["state"].as<std::string>();
    card.heading = row["heading"].as<std::string>();
    return card;
}

Card get_duplicate_card(pqxx::row const& row)
{
    Card card;

    if (row.size())
    {
        card.subject_id = row["rid"].as<uint64_t>();
        card.card_id = row["card"].as<uint64_t>();
        card.topic_position = row["sid"].as<uint32_t>();
        card.card_position = row["position"].as<uint32_t>();
        card.subject = row["resource"].as<std::string>();
        card.topic = row["section"].as<std::string>("");
        card.state = row["state"].as<std::string>();
        card.heading = row["heading"].as<std::string>();
    }
    return card;
}

void merge_cards(int const tcard, int const scard)
{
    pqxx::work merge{connection};
    merge.exec("call merge_cards($1, $2)", {scard, tcard});
    merge.commit();
}

int main()
{
    pqxx::nontransaction tx{connection};
    pqxx::result lost_cards{tx.exec(lost_cards_query)};
    tx.commit();

    for (pqxx::row const& row: lost_cards)
    {
        Card topic_card{get_lost_card(row)};
        pqxx::work w{connection};
        pqxx::result duplicate_cards{w.exec(duplicate_card_query, topic_card.card_id)};
        int duplicates{duplicate_cards.size()};
        w.commit();

        if (duplicates == 0)
        {
            std::println("Card \033[0;1;31m{}\033[0m in topic \033[0;1;2;31m{} {}\033[0m of subject \033[0;1;2;31m{} {}\033[0m has no duplicates\n\n\033[0;1;35m{}\033[0m\n",
                topic_card.card_id,
                topic_card.topic_position,
                topic_card.topic,
                topic_card.subject_id,
                topic_card.subject,
                topic_card.heading
            );
            std::ostringstream content{};
            pqxx::work block_transaction{connection};
            pqxx::result blocks{block_transaction.exec(get_blocks_query, topic_card.card_id)};
            for (pqxx::row const& block: blocks)
            {
                content << block["content"].as<std::string>() << "\n";
            }
            block_transaction.commit();
            std::println("{}\n---\n", content.str());
        }
        else
        {
            for (pqxx::row const& duplicate: duplicate_cards)
            {
                Card section_card{get_duplicate_card(duplicate)};
                std::println("Card \033[0;1;31m{}\033[0m in topic \033[0;1;2;31m{} {}\033[0m of subject \033[0;1;2;31m{} {}\033[0m is a duplicate of\nCard \033[0;1;33m{}\033[0m in section \033[0;1;2;33m{} {}\033[0m of resource \033[0;1;2;33m{} {}\033[0m\n\n\033[0;1;35m{}\033[0m",
                    topic_card.card_id,
                    topic_card.topic_position,
                    topic_card.topic,
                    topic_card.subject_id,
                    topic_card.subject,
                    section_card.card_id,
                    section_card.topic_position,
                    section_card.topic,
                    section_card.subject_id,
                    section_card.subject,
                    topic_card.heading
                );

                std::ostringstream tblock{};
                std::ostringstream sblock{};

                pqxx::work b{connection};
                pqxx::result blocks{b.exec(get_blocks_query, topic_card.card_id)};
                for (pqxx::row const& block: blocks)
                {
                    tblock << block["content"].as<std::string>();
                    //println("{}\n", block["content"].as<std::string>());
                }
                //std::println("---");
                b.commit();

                pqxx::result sblocks{b.exec(get_blocks_query, section_card.card_id)};
                for (pqxx::row const& block: blocks)
                {
                    sblock << block["content"].as<std::string>();
                    //std::println("{}\n", block["content"].as<std::string>());
                }
                //std::println("---");
                b.commit();

                if (sblock.str() == tblock.str())
                {
                    std::println("{}\n", "with identical blocks");
                    try
                    {
                        merge_cards(topic_card.card_id, section_card.card_id);
                    }
                    catch (pqxx::unique_violation const& err)
                    {
                        std::println("\033[0;1;31mSection card {} already exists in the topic, removing duplicate topic card {}\033[0m", section_card.card_id, topic_card.card_id);
                        pqxx::work remove{connection};
                        remove.exec("delete from topics_cards where subject = $1 and topic = $2 and level = $3 and card = $4", {topic_card.subject_id, topic_card.topic_position, topic_card.level, topic_card.card_id});
                        remove.commit();
                    }
                }
                else
                {
                    std::println("Blocks of section card {}\n\n{}\n---", section_card.card_id, sblock.str());
                    std::println("Blocks of topic card {}\n\n{}\n---", topic_card.card_id, tblock.str());
                    std::print("Merge {} and {}? ", section_card.card_id, topic_card.card_id);
                    std::string response{};
                    std::cin >> response;

                    if (response == "y")
                    {
                        merge_cards(topic_card.card_id, section_card.card_id);
                    }
                }
            }
        }
    }
}
