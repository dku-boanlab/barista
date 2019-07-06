CONFIG= \
    __DEFAULT_PORT=6633 \
    \
    __CLI_ALLOW_CONNECT=\"127.0.0.1\" \
    __CLI_PORT=8000 \
    \
    __EXT_COMP_PULL_ADDR=\"tcp://127.0.0.1:5001\" \
    __EXT_COMP_REPLY_ADDR=\"tcp://127.0.0.1:5002\" \
    \
    __EXT_APP_PULL_ADDR=\"tcp://127.0.0.1:6001\" \
    __EXT_APP_REPLY_ADDR=\"tcp://127.0.0.1:6002\" \
    \
    __MAX_COMPONENTS=20 \
    __MAX_EVENTS=80 \
    __MAX_EVENT_CHAINS=10 \
    \
    __MAX_APPLICATIONS=10 \
    __MAX_APP_EVENTS=40 \
    __MAX_APP_EVENT_CHAINS=10 \
    \
    __MAX_POLICIES=10 \
    __MAX_POLICY_ENTRIES=10 \
    \
    __NUM_PULL_THREADS=4 \
    __NUM_REP_THREADS=2 \
    \
    __MAX_NUM_SWITCHES=128 \
    __MAX_NUM_PORTS=64 \
    \
    #__ENABLE_CBENCH \
    #\
    #__ENABLE_META_EVENTS \
    #__MAX_META_EVENTS=10 \
