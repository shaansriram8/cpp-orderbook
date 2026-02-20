#include <optional>
#include <cstdint>
#include <map>
#include <deque>
#include <functional>
#include <unordered_map>
#include <variant>

enum class Side
{
    Buy,
    Sell
};

// request to place an order
struct Order
{
    uint64_t id; // unique identifier for order
    uint64_t price;
    uint32_t quantity;
    Side side;
};

// Each ticker owns its own OrderBook. Each OrderBook manages all resting orders for that ticker.
class OrderBook
{
public:
    enum class PlaceResult
    {
        Accepted,
        DuplicateId,
        InvalidPrice,
        InvalidQuantity
    };

    enum class CancelResult
    {
        Canceled,
        NotFound
    };

    PlaceResult placeOrder(const Order &newOrder);

    // for cancel order, we want:
    //   -O(1) lookup by ID, O(1) removal from the price level
    //   -if price becomes empty, remove it
    //   -remove id from index
    CancelResult cancelOrder(uint64_t id);

    std::optional<uint64_t> getBestBid() const;                 // if book is empty, return optional
    std::optional<uint64_t> getBestAsk() const;                 // if book is empty, return optional
    uint32_t getVolumeAtPrice(Side side, uint64_t price) const; // get market depth

private:
    uint64_t nextSequence_ = 0; // represents next sequence number that will be assigned (internal counter)

    struct InternalOrder
    {
        Order order;
        uint64_t sequence; // relative order, immutable
    };

    // Type aliases to avoid comparator type mismatch

    // Maps: enforces price ordering -> FIFO within each price
    using BidMap = std::map<uint64_t, std::deque<InternalOrder>, std::greater<uint64_t>>;
    using AskMap = std::map<uint64_t, std::deque<InternalOrder>>;

    struct OrderLocation
    {
        Side side; // buy/sell
        std::variant<BidMap::iterator, AskMap::iterator> priceLevelIt;
        std::deque<InternalOrder>::iterator orderIt;
    };

    BidMap bids_; // price: key, deque: FIFO, std::greater: descending order
    AskMap asks_; // default comparator: ascending

    // ID lookup table:
    //   -if not found -> return NotFound
    //   -if found -> return OrderLocation
    std::unordered_map<uint64_t, OrderLocation> orderIndex_;

    void matchIncoming(InternalOrder &incomingOrder);
};