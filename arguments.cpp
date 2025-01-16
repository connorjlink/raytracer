import std;

#include "arguments.h"
#include "log.h"

namespace
{
	std::vector<std::string> split(std::string text, const std::string& delimiter)
	{
		return text
			| std::ranges::views::split(delimiter)
			| std::ranges::views::transform([](auto&& str) 
				{ return std::string_view(&*str.begin(), std::ranges::distance(str)); })
			| std::ranges::to<std::vector<std::string>>();
	}

	std::vector<std::string> split2(const std::string& text, char delimiter)
	{
		std::vector<std::string> out{};

		const auto pos = text.find_first_of(delimiter);
		out.emplace_back(text.substr(0, pos));
		out.emplace_back(text.substr(pos + 1));
		
		return out;
	}

	std::vector<std::string> convert(int argc, char** argv)
	{
		std::vector<std::string> arguments{ static_cast<std::size_t>(argc - 1) };

		if (argc > 0)
		{
			for (auto i = 1; i < argc; i++)
			{
				arguments[i - 1] = argv[i];
			}
		}

		return arguments;
	}

    struct ProcessResult
    {
        bool success;
        int result;
    };

    ProcessResult parse_integer(const std::string& input)
    {
	    int result = 0;
	    const char* start = input.data();
	    const char* end = start + input.size();

	    auto [ptr, ec] = std::from_chars(start, end, result);

	    bool success = (ec == std::errc() && ptr == end);
	    return { success, result };
	}
}

namespace
{
    enum class ArgumentType
    {
        WIDTH,
        HEIGHT,
        MODE,
        BOUNCES,
    };

    static const std::unordered_map<std::string, ArgumentType> _arguments_map
    {
        { "width", ArgumentType::WIDTH },
        { "height", ArgumentType::HEIGHT },
        { "mode", ArgumentType::MODE },
        { "bounces", ArgumentType::BOUNCES },
    };
}

namespace luma
{
	using namespace std::chrono_literals;

	void Arguments::parse(int argc, char** argv)
	{
		const auto arguments = convert(argc, argv);
		
		for (auto& argument : arguments)
		{
			// parsing a command-line option
			if (argument.starts_with("--"))
			{
				const auto argument_unprefixed = argument.substr(2);

				const auto argument_split = ::split2(argument_unprefixed, '=');

				if (argument_split.size() != 2)
				{
					log(std::format("unrecognized option format `{}`", argument));
					continue;
				}

				const auto& option_string = argument_split[0];

				if (!_arguments_map.contains(option_string))
				{
                    log(std::format("unrecognized option `{}`", option_string));
					continue;
				}

				const auto option = _arguments_map.at(option_string);
				const auto& value = argument_split[1];

				using enum ArgumentType;
				switch (option)
				{
                    case WIDTH:
                    {
                        const auto[success, result] = parse_integer(value);

                        if (!success)
                        {
							log(std::format("unrecognized image width `{}`", value));
							continue;
                        }

						_options.width = result;
                    } break;

                    case HEIGHT:
                    {
						const auto [success, result] = parse_integer(value);

						if (!success)
						{
							log(std::format(""))
						}

                    } break;

                    case MODE:
                    {
                        if (!_mode_map.contains(value))
                        {
                            log(std::format("unrecognized mode `{}`", value));
                            continue;
                        }

                        _options->_mode = _mode_map.at(value);
                    } break;

                    case BOUNCES:
                    {
                    } break;

					case ARCHITECTURE:
					{
						if (!_architecture_map.contains(value))
						{
							_error_reporter->post_warning(std::format("unrecognized architecture type `{}`", value), NULL_TOKEN);
							continue;
						}

						_options->_architecture = _architecture_map.at(value);
					} break;

					case VERBOSITY:
					{
						if (!_verbosity_type_map.contains(value))
						{
							_error_reporter->post_warning(std::format("unrecognized verbosity type `{}`", value), NULL_TOKEN);
							continue;
						}

						_options->_verbosity = _verbosity_type_map.at(value);
					} break;

					case EXECUTION:
					{
						if (!_execution_map.contains(value))
						{
							_error_reporter->post_warning(std::format("unrecognized execution type `{}`", value), NULL_TOKEN);
							continue;
						}

						_options->_execution = _execution_map.at(value);
					} break;

					case OUTPUT:
					{
						if (value != "raw")
						{
							_error_reporter->post_warning(std::format("unrecognized output type `{}`", value), NULL_TOKEN);
							continue;
						}
					} break;

					case OPTIMIZATION:
					{
						if (!_optimization_map.contains(value))
						{
							_error_reporter->post_warning(std::format("unrecognized optimization type `{}`", value), NULL_TOKEN);
							continue;
						}

						auto flag = _optimization_map.at(value);
						_options->_optimization |= flag;
					} break;
				}
			}

			// parsing a source code filepath
			else
			{
				_filepaths.emplace_back(argument);
			}
		}
	}
}