module;

#include <cli/cli.hh>

export module cli20;

export namespace cli {
using cli::ColorMode;
using cli::DoubleOption;
using cli::Flag;
using cli::Help;
using cli::IntOption;
using cli::ListOption;
using cli::Option;
using cli::Parser;
using cli::ParseResult;
using cli::PathOption;
using cli::Positional;
using cli::RecurseHelpTag;
using cli::StringOption;
using cli::Token;
using cli::TokenizerConfig;
using cli::TokenizeResult;

using cli::Action;
using cli::ActionCtx;
using cli::ActionResult;
using cli::ArgImpl;
using cli::ArgParameter;
using cli::ArgumentSpec;
using cli::Command;
using cli::CommandParameter;
using cli::CommandTag;
using cli::Description;
using cli::DescriptionTag;
using cli::ErrorCode;
using cli::ErrorKind;
using cli::Nargs;
using cli::OptionTag;
using cli::ParseError;
using cli::PositionalImpl;
using cli::PositionalTag;
using cli::Presence;
using cli::SpecMemberTag;
using cli::StringLiteral;

using cli::as_tuple;
using cli::format_help;
using cli::optional;
using cli::parse;
using cli::parse_or_exit;
using cli::recurseHelp;
using cli::required;
using cli::to_string;
using cli::tokenize;

namespace conversion {
using cli::conversion::Bool;
using cli::conversion::Choice;
using cli::conversion::Enumeration;
using cli::conversion::ExistingDirectory;
using cli::conversion::ExistingFile;
using cli::conversion::Floating;
using cli::conversion::Integer;
using cli::conversion::Path;
using cli::conversion::String;

using cli::conversion::boolean;
using cli::conversion::choice;
using cli::conversion::enumeration;
using cli::conversion::default_missing_value;
using cli::conversion::existing_directory;
using cli::conversion::existing_file;
using cli::conversion::floating;
using cli::conversion::integer;
using cli::conversion::path;
using cli::conversion::string;
}  // namespace conversion

namespace validation {
using cli::validation::Exists;
using cli::validation::IsDirectory;
using cli::validation::IsRegularFile;
using cli::validation::Matches;
using cli::validation::Max;
using cli::validation::Min;
using cli::validation::NonEmpty;
using cli::validation::NonNegative;
using cli::validation::NotBlank;
using cli::validation::OneOf;
using cli::validation::ParentExists;
using cli::validation::Positive;
using cli::validation::Predicate;
using cli::validation::Range;

using cli::validation::exists;
using cli::validation::is_directory;
using cli::validation::is_regular_file;
using cli::validation::matches;
using cli::validation::max;
using cli::validation::min;
using cli::validation::non_empty;
using cli::validation::non_negative;
using cli::validation::not_blank;
using cli::validation::one_of;
using cli::validation::parent_exists;
using cli::validation::positive;
using cli::validation::predicate;
using cli::validation::range;
}  // namespace validation

namespace pack {
using cli::pack::Callback;
using cli::pack::Extend;
using cli::pack::Increment;
using cli::pack::Insert;
using cli::pack::InsertOrAssign;
using cli::pack::MarkPresent;
using cli::pack::Push;
using cli::pack::PushUnique;
using cli::pack::RejectDuplicate;
using cli::pack::SetFalse;
using cli::pack::SetOnce;
using cli::pack::SetTrue;
using cli::pack::Toggle;

using cli::pack::callback;
using cli::pack::extend;
using cli::pack::increment;
using cli::pack::insert;
using cli::pack::insert_or_assign;
using cli::pack::mark_present;
using cli::pack::push;
using cli::pack::push_unique;
using cli::pack::reject_duplicate;
using cli::pack::set_false;
using cli::pack::set_once;
using cli::pack::set_true;
using cli::pack::toggle;
}  // namespace pack

namespace action {
using cli::action::ExitSuccess;
using cli::action::PrintHelp;

using cli::action::exit_success;
using cli::action::print_help;
}  // namespace action

namespace nargs {
using cli::nargs::between;
using cli::nargs::exactly;
using cli::nargs::none;
using cli::nargs::one;
using cli::nargs::one_or_more;
using cli::nargs::zero_or_more;
using cli::nargs::zero_or_one;
}  // namespace nargs
}  // namespace cli
