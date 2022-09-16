#include "padrack.h"

int padrack::getQuantity()
{
    return quantity;
}
void padrack::setQuantity(int number)
{
    quantity = number;
}
void padrack::decQuantity()
{
    if (quantity != 0)
    {
        quantity--;
    }
}
void padrack::setZero()
{
    if (quantity != 0)
    {
        quantity = 0;
    }
}
void padrack::incQuantity()
{
    quantity++;
    if (quantity > 60)
    {
        setQuantity(0);
    }
}
