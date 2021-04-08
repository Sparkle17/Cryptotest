#pragma once
#include <map>
#include <memory>
#include <shared_mutex>
#include <vector>

#include "..\PreciseNumber.h"

using namespace std;

typedef pair<PreciseNumber, PreciseNumber> LotInfo;

class BookLoader: public enable_shared_from_this<BookLoader>
{
public:
    class IUpdateHandler
    {
    public:
        virtual void onBookUpdate(const BookLoader* book) = 0;
    };

private:
	bool m_disconnected;

protected:
    template <typename Derived>
    shared_ptr<Derived> shared_from_base()
    {
        return static_pointer_cast<Derived>(shared_from_this());
    }

    map<PreciseNumber, PreciseNumber> m_sells;
    map<PreciseNumber, PreciseNumber> m_buys;
    mutable shared_mutex m_mutex;

    string m_symBase;
    string m_symTrade;
    string m_pair;
    PreciseNumber m_minLotSize;
    PreciseNumber m_notion;

    PreciseNumber m_bestAsk;
    PreciseNumber m_bestAskQuantity;
    PreciseNumber m_bestBid;
    PreciseNumber m_bestBidQuantity;

    mutable shared_mutex m_updateMutex;
    vector<IUpdateHandler*> m_updateHandlers;

    void onUpdate();

public:
    BookLoader(const string& symBase, const string& symTrade);
	virtual ~BookLoader();

    LotInfo bestBuy(int skip = 0) const;
    LotInfo bestSell(int skip = 0) const;

    virtual void connect();
	virtual void disconnect();
   
    void registerUpdateHandler(IUpdateHandler* handler);
    void unregisterUpdateHandler(IUpdateHandler* handler);

	bool isDisconnected() const { return m_disconnected; }

    string getPair() const { return m_pair; }
    string getBaseCoin() const { return m_symBase; }
    string getTradeCoin() const { return m_symTrade; }
    PreciseNumber getMinLotSize() const { return m_minLotSize; }
    PreciseNumber getNotion() const { return m_notion; }
};
