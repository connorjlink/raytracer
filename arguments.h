#ifndef LUMA_ARGUMENTS_H
#define LUMA_ARGUMENTS_H

namespace luma
{
    class Arguments
    {
    private:
        std::vector<std::string> _filepaths;

    public:
        const decltype(_filepaths)& files() const
        {
            return _filepaths;
        }

    public:
        Arguments()
        {
        }

    public:
        void parse(int, char**);
    };
}

#endif