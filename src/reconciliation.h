/** Static component of the salt used to compute short txids for transaction reconciliation. */
static const std::string RECON_STATIC_SALT = "Tx Relay Salting";
/** Used to convert a floating point reconciliation coefficient q to an int for transmission. Specified by BIP-330. */
static constexpr uint16_t Q_PRECISION{(2 << 14) - 1};
/**
 * Interval between sending reconciliation request to the same peer.
 * This value allows to reconcile ~100 transactions (7 tx/s * 16s) during normal system operation at capacity.
 * More frequent reconciliations would cause significant constant bandwidth overhead due to
 * reconciliation metadata (sketch sizes etc.), which would nullify the efficiency.
 * Less frequent reconciliations would introduce high transaction relay latency.
 */
static constexpr std::chrono::microseconds RECON_REQUEST_INTERVAL{16s};
/**
 * Used to keep track of the current reconciliation round with a peer.
 * Used for both inbound (responded) and outgoing (requested/initiated) reconciliations.
 */
enum ReconPhase {
    RECON_NONE,
    RECON_INIT_REQUESTED,
    RECON_INIT_RESPONDED,
    RECON_EXT_REQUESTED,
    RECON_EXT_RESPONDED,
};

/**
 * This struct is used to keep track of the reconciliations with a given peer,
 * and also short transaction IDs for the next reconciliation round.
 * Transaction reconciliation means an efficient synchronization of the known
 * transactions between a pair of peers.
 * One reconciliation round consists of a sequence of messages. The sequence is
 * asymmetrical, there is always a requestor and a responder. At the end of the
 * sequence, nodes are supposed to exchange transactions, so that both of them
 * have all relevant transactions. For more protocol details, refer to BIP-0330.
 */
struct ReconState {
    /** Default coefficient used to estimate set difference for tx reconciliation. */
    static constexpr double DEFAULT_RECON_Q = 0.02;

    ReconState(bool requestor, bool responder, bool flood_to, uint64_t k0, uint64_t k1) :
        m_requestor(requestor), m_responder(responder), m_flood_to(flood_to), m_k0(k0), m_k1(k1), m_local_q(DEFAULT_RECON_Q) {}

    /// Whether this peer will send reconciliation requests.
    bool m_requestor;

    /// Whether this peer will respond to reconciliation requests.
    bool m_responder;

    /**
     * Since reconciliation-only approach makes transaction relay
     * significantly slower, we also announce some of the transactions
     * (currently, transactions received from inbound links)
     * to some of the peers:
     * - all pre-reconciliation peers supporting transaction relay;
     * - a limited number of outbound reconciling peers *for which this flag is enabled*.
     * We enable this flag based on whether we have a
     * sufficient number of outbound transaction relay peers.
     * This flooding makes transaction relay across the network faster
     * without introducing high the bandwidth overhead.
     * Transactions announced via flooding should not be added to
     * the reconciliation set.
     */
    bool m_flood_to;

    /**
     * Reconciliation involves computing and transmitting sketches,
     * which is a bandwidth-efficient representation of transaction IDs.
     * Since computing sketches over full txID is too CPU-expensive,
     * they will be computed over shortened IDs instead.
     * These short IDs will be salted so that they are not the same
     * across all pairs of peers, because otherwise it would enable network-wide
     * collisions which may (intentionally or not) halt relay of certain transactions.
     * Both of the peers contribute to the salt.
     */
    const uint64_t m_k0, m_k1;

    /**
     * Computing a set reconciliation sketch involves estimating the difference
     * between sets of transactions on two sides of the connection. More specifically,
     * a sketch capacity is computed as
     * |set_size - local_set_size| + q * (set_size + local_set_size) + c,
     * where c is a small constant, and q is a node+connection-specific coefficient.
     * This coefficient is recomputed by every node based on its previous reconciliations,
     * to better predict future set size differences.
     */
    double m_local_q;

    /**
     * Store all transactions which we would relay to the peer (policy checks passed, etc.) in this set
     * instead of announcing them right away. When reconciliation time comes, we will
     * compute an efficient representation of this set ("sketch") and use it to efficient reconcile
     * this set with a similar set on the other side of the connection.
     */
    std::set<uint256> m_local_set;

    /** Keep track of the outgoing reconciliation with the peer. */
    ReconPhase m_outgoing_recon{RECON_NONE};
};
