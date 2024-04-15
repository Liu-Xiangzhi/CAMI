
int main()
{
    int* p;
    {
        int x = 0;
        p = &x;
    }
    (void)p;
}
