#include "BookLoader.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>

using namespace std;

BookLoader::BookLoader(const string& symBase, const string& symTrade)
{
	m_symBase = boost::to_upper_copy(symBase);
	m_symTrade = boost::to_upper_copy(symTrade);
	m_pair = m_symBase + m_symTrade;
}

BookLoader::~BookLoader()
{
}

LotInfo BookLoader::bestBuy(int skip) const
{
	if (skip == 0)
		return LotInfo(m_bestBid, m_bestBidQuantity);
	shared_lock<shared_mutex> rLock(m_mutex);
	if (!m_buys.empty()) {
		auto it = m_buys.rbegin();
		for (int i = 0; i < skip; i++)
			++it;
		if (it != m_buys.rend())
			return *it;
	}
	return LotInfo(0, 0);
}

LotInfo BookLoader::bestSell(int skip) const
{
	if (skip == 0)
		return LotInfo(m_bestAsk, m_bestAskQuantity);
	shared_lock<shared_mutex> rLock(m_mutex);
	if (!m_sells.empty()) {
		auto it = m_sells.begin();
		for (int i = 0; i < skip; i++)
			++it;
		if (it != m_sells.end())
			return *it;
	}
	return LotInfo(0, 0);
}

void BookLoader::disconnect()
{
	m_disconnected = true;
}

void BookLoader::connect()
{
	m_disconnected = false;
}

void BookLoader::onUpdate()
{
	shared_lock<shared_mutex> lock(m_updateMutex);
	for (auto& handler : m_updateHandlers)
		handler->onBookUpdate(this);
}

void BookLoader::registerUpdateHandler(IUpdateHandler* handler)
{
	lock_guard<shared_mutex> lock(m_updateMutex);
	m_updateHandlers.push_back(handler);
}

void BookLoader::unregisterUpdateHandler(IUpdateHandler* handler)
{
	lock_guard<shared_mutex> lock(m_updateMutex);
	auto it = find(m_updateHandlers.begin(), m_updateHandlers.end(), handler);
	if (it != m_updateHandlers.end())
		m_updateHandlers.erase(it);
}
