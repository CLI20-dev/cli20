module;

#include <cli/cli.hh>

export module cli20;

export namespace cli {
    using cli::Flag;
    using cli::Help;
    using cli::Option;
    using cli::ListOption;
    using cli::Positional;
    using cli::StringOption;
    using cli::IntOption;
    using cli::DoubleOption;
    using cli::PathOption;
    using cli::ColorMode;
    using cli::RecurseHelpTag;
    using cli::Token;
    using cli::TokenizeResult;
    using cli::TokenizerConfig;
    using cli::ParseResult;
    using cli::Parser;

    using cli::ErrorCode;
    using cli::ErrorKind;
    using cli::ParseError;
    using cli::StringLiteral;
    using cli::ActionResult;
    using cli::ActionCtx;
    using cli::Action;
    using cli::SpecMemberTag;
    using cli::OptionTag;
    using cli::PositionalTag;
    using cli::CommandTag;
    using cli::DescriptionTag;
    using cli::Nargs;
    using cli::Presence;
    using cli::ArgumentSpec;
    using cli::ArgParameter;
    using cli::ArgImpl;
    using cli::PositionalImpl;
    using cli::Description;
    using cli::CommandParameter;
    using cli::Command;

    using cli::toString;
    using cli::recurseHelp;
    using cli::formatHelp;
    using cli::as_tuple;
    using cli::required;
    using cli::optional;
    using cli::tokenize;
    using cli::parse;
    using cli::parseOrExit;

    namespace conversion {
        using cli::conversion::Integer;
        using cli::conversion::Floating;
        using cli::conversion::String;
        using cli::conversion::Bool;
        using cli::conversion::Path;
        using cli::conversion::ExistingFile;
        using cli::conversion::ExistingDirectory;
        using cli::conversion::Choice;
        using cli::conversion::Enumeration;

        using cli::conversion::integer;
        using cli::conversion::floating;
        using cli::conversion::choice;
        using cli::conversion::enumeration;
        using cli::conversion::string;
        using cli::conversion::boolean;
        using cli::conversion::path;
        using cli::conversion::existing_file;
        using cli::conversion::existing_directory;
    } // namespace conversion

    namespace validation {
        using cli::validation::Min;
        using cli::validation::Max;
        using cli::validation::Range;
        using cli::validation::Positive;
        using cli::validation::NonNegative;
        using cli::validation::NonEmpty;
        using cli::validation::NotBlank;
        using cli::validation::OneOf;
        using cli::validation::Matches;
        using cli::validation::Exists;
        using cli::validation::IsRegularFile;
        using cli::validation::IsDirectory;
        using cli::validation::ParentExists;
        using cli::validation::Predicate;

        using cli::validation::min;
        using cli::validation::max;
        using cli::validation::range;
        using cli::validation::one_of;
        using cli::validation::matches;
        using cli::validation::predicate;
        using cli::validation::positive;
        using cli::validation::non_negative;
        using cli::validation::non_empty;
        using cli::validation::not_blank;
        using cli::validation::exists;
        using cli::validation::is_regular_file;
        using cli::validation::is_directory;
        using cli::validation::parent_exists;
    } // namespace validation

    namespace pack {
        using cli::pack::SetTrue;
        using cli::pack::SetFalse;
        using cli::pack::Toggle;
        using cli::pack::Increment;
        using cli::pack::RejectDuplicate;
        using cli::pack::SetOnce;
        using cli::pack::PushUnique;
        using cli::pack::Push;
        using cli::pack::Insert;
        using cli::pack::InsertOrAssign;
        using cli::pack::Extend;
        using cli::pack::MarkPresent;
        using cli::pack::Callback;

        using cli::pack::set_true;
        using cli::pack::set_false;
        using cli::pack::toggle;
        using cli::pack::increment;
        using cli::pack::set_once;
        using cli::pack::reject_duplicate;
        using cli::pack::push_unique;
        using cli::pack::push;
        using cli::pack::insert;
        using cli::pack::insert_or_assign;
        using cli::pack::extend;
        using cli::pack::mark_present;
        using cli::pack::callback;
    } // namespace pack

    namespace action {
        using cli::action::PrintHelp;
        using cli::action::ExitSuccess;

        using cli::action::print_help;
        using cli::action::exit_success;
    } // namespace action

    namespace nargs {
        using cli::nargs::none;
        using cli::nargs::one;
        using cli::nargs::zero_or_one;
        using cli::nargs::zero_or_more;
        using cli::nargs::one_or_more;
        using cli::nargs::exactly;
        using cli::nargs::between;
    } // namespace nargs
} // namespace cli
