
#include <Trade\Trade.mqh>
#include <Trade\PositionInfo.mqh>
CTrade trade;
CPositionInfo Position;

//class for holding derivative values
class values {
public:
    int askder;
    int bidder;
    values() {
        askder = 0;
        bidder = 0;
    }
};

//class for holding binomialderivative values 
class binomialderivative {
public:
    int askbinomialderivative;
    int bidbinomialderivative;
    binomialderivative() {
        askbinomialderivative = 0;
        bidbinomialderivative = 0;
    }
};

//class for storing binomials

class currentprices {
public:
    double askbinomial;
    double bidbinomial;
    currentprices() {
        askbinomial = 0;
        bidbinomial = 0;
    }
    currentprices(double ask, double bid) {
        askbinomial = ask;
        bidbinomial = bid;
    }
    currentprices(currentprices& d) {
        askbinomial = d.askbinomial;
        bidbinomial = d.bidbinomial;
    }
};

//global values here 
input int countersize = 50;
input double tradevolume = 0.01;
values derivativearray[10000000];
binomialderivative binomialderivativearray[10000000];
int arraysize;

//initializing data points for previous ontick call
static ulong lasttime;
static double lastaskprice;
static double lastbidprice;
static double lastaskbinomial;
static double lastbidbinomial;

//initialising data points for current ontick call
static ulong currenttime;
static double currentaskprice;
static double currentbidprice;

//initialising static variables to hold the bayesian probability of selling and buying
double probabilitytosellinmarket;
double probabilitytobuyinmarket;

//the function that returns the binomial of
double factorial(double number) {
    double fact = 1;
    if ((number == 0) || (number == 1)) {
        return fact;
    }
    else {
        return number * factorial(number - 1);
    }
}

//function that returns binomial coefficient 
double binomialcoefficient(double upevents) {
    double numerator = factorial(countersize);
    double denominator = factorial(upevents) * factorial((countersize - upevents));
    return (numerator / denominator);
}

//function that returns the probability of the whole chosen array happening

double binomialprobability(double upevents, double upprobability) {
    double coefficient = binomialcoefficient(upevents);
    double probabilityofevent = ((MathPow(upprobability, upevents)) * (MathPow((1 - upprobability), (countersize - upevents))));
    return (coefficient * probabilityofevent);
}

//function to calculate probabilities

currentprices probabilityofcountersizeevents() {
    int array = arraysize;
    double asktotalup = 0;
    double bidtotalup = 0;
    for (int start = array - countersize; start <= array; start++) {
        asktotalup += derivativearray[start].askder;
        bidtotalup += derivativearray[start].bidder;
    }

    double askkkk = binomialprobability(asktotalup, probabilitytobuyinmarket);
    double bidddd = binomialprobability(bidtotalup, probabilitytobuyinmarket);
    currentprices a(askkkk, bidddd);
    return a;
}


//this function closes position sspecified by the inputt string
void CloseOpenPositions()
{
    for (int i = PositionsTotal() - 1; i >= 0; i--)
    {
        if (Position.SelectByIndex(i))
        {
            if (Position.Symbol() == Symbol())
            {
                bool profitable = (PositionGetDouble(POSITION_PROFIT) > 0) ? true : false;
                bool bigloss = ((MathAbs((PositionGetDouble(POSITION_PROFIT))) > 0.05) && ((PositionGetDouble(POSITION_PROFIT)) < 0)) ? true : false;
                if ((profitable) || (bigloss)) {
                    trade.PositionClose(Position.Ticket());
                }
            }
        }
    }
}

void CloseSpecificPositions(int decider)
{
    for (int i = PositionsTotal() - 1; i >= 0; i--)
    {
        if (Position.SelectByIndex(i))
        {
            if (Position.Symbol() == Symbol())
            {
                bool buyy = ((PositionGetInteger(POSITION_TYPE) == ORDER_TYPE_BUY) && (decider == 1)) ? true : false;
                bool selll = ((PositionGetInteger(POSITION_TYPE) == ORDER_TYPE_SELL) && (decider == 2)) ? true : false;
                if (buyy) {
                    trade.PositionClose(Position.Ticket());
                }
                if (selll) {
                    trade.PositionClose(Position.Ticket());
                }
            }
        }
    }
}

int OnInit()
{
    //initialization of the statoc variables at the start of teh expert advisor 
    lastaskbinomial = 0;
    lastbidbinomial = 0;
    lastaskprice = 0;
    lastbidprice = 0;
    currentbidprice = 0;
    currentaskprice = 0;
    currenttime = 0;
    lasttime = 0;
    arraysize = 0;
    probabilitytobuyinmarket = 0.5; //in later iterations of the EA, we will also update this [robability using baayesian ways 
    probabilitytosellinmarket = 0.5; //in later iterations of teh model we will update the probabilities using bayesioan


    return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason)
{


}


void OnTick()
{
    //---
    //getting all the current data that we need as the tick just starts 
    currentaskprice = SymbolInfoDouble(Symbol(), SYMBOL_ASK);
    currentbidprice = SymbolInfoDouble(Symbol(), SYMBOL_BID);
    currenttime = GetMicrosecondCount();

    //finding our derived units from the initial data 
    ulong timeelapsed = (currenttime - lasttime) * 1000000;
    double askdifference = currentaskprice - lastaskprice;
    double biddifference = currentbidprice - lastbidprice;

    //calculating the porice derivatives of the values 
    double askderivative = askdifference / timeelapsed * 1000;
    double bidderivative = biddifference / timeelapsed * 1000;



    values now;
    now.askder = (askderivative > 0) ? 1 : 0;
    now.bidder = (bidderivative > 0) ? 1 : 0;
    int x = arraysize;
    derivativearray[x] = now;

    int wheretostart = countersize;
    wheretostart += 20;
    bool startcalculatingprobabilities = (arraysize >= wheretostart) ? true : false;

    if (startcalculatingprobabilities) {

        double down = 0;
        for (int i = (arraysize - countersize); i <= arraysize; i++) {
            down += derivativearray[i].askder;
        }

        down /= countersize;
        double up = (1 - down);

        CloseOpenPositions();
        currentprices b;
        binomialderivative currentbinomialderivatives;
        b = probabilityofcountersizeevents();
        currentbinomialderivatives.askbinomialderivative = (b.askbinomial > lastaskbinomial) ? 1 : 0;
        currentbinomialderivatives.bidbinomialderivative = (b.bidbinomial > lastbidbinomial) ? 1 : 0;
        lastaskbinomial = b.askbinomial;
        lastbidbinomial = b.bidbinomial;
        bool buynow = (((down > up) && (currentbinomialderivatives.askbinomialderivative == 1)) || ((down < up) && (currentbinomialderivatives.askbinomialderivative == 0))) ? true : false;
        bool sellnow = (((up > down) && (currentbinomialderivatives.askbinomialderivative == 1)) || ((up < down) && (currentbinomialderivatives.askbinomialderivative == 0))) ? true : false;

        if (buynow) {
            trade.Buy(tradevolume, _Symbol, currentaskprice, NULL, NULL, NULL);
        }

        if (sellnow) {
            trade.Sell(tradevolume, _Symbol, currentbidprice, NULL, NULL, NULL);
        }
    }

    lasttime = currenttime;
    lastaskprice = currentaskprice;
    lastbidprice = currentbidprice;
    arraysize += 1;
}
