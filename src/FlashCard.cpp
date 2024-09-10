#include "FlashCard.hpp"

FlashCard * allocFlashCard(Card card, std::string a, std::string q)
{
    FlashCard *flash_card = new FlashCard();
    flash_card->card = card;
    flash_card->a = a;
    flash_card->q = q;

    return flash_card;
}

void freeFlashCard(FlashCard *card)
{
    delete card;
}
