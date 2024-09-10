#ifndef FLASHCARD_HPP
#define FLASHCARD_HPP

#include "models.hpp"

enum FlashCardStatus {
    NONE = 0,
    AGAIN,
    HARD,
    GOOD,
    EASY,
    NumFlashCardStatus
};

struct FlashCard {
    Card card;
    std::string q;
    std::string a;
};

FlashCard * allocFlashCard(Card card, std::string a, std::string q);
void freeFlashCard(FlashCard *card);

#endif
