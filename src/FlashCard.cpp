#include "FlashCard.hpp"

FlashCard * allocFlashCard(int uuid, Card card, std::string a, std::string q)
{
    FlashCard *flash_card = new FlashCard();
    flash_card->uuid = uuid;
    flash_card->card = card;
    flash_card->a = a;
    flash_card->q = q;

    return flash_card;
}

void freeFlashCard(FlashCard *card)
{
    delete card;
}
