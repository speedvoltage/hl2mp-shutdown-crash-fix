extern void __gxx_personality_v0(void);

void *g_mock_personality_slot = (void *)&__gxx_personality_v0;

void mock_set_personality(void *address)
{
    g_mock_personality_slot = address;
}

void *mock_get_personality(void)
{
    return g_mock_personality_slot;
}
