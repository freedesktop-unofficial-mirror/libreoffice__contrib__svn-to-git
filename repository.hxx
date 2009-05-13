#ifndef _REPOSITORY_HXX_
#define _REPOSITORY_HXX_

#include <string>
#include <fstream>

#include <regex.h>

#define TAG_TEMP_BRANCH "tag-branches/"

class Committer;

struct Tag
{
    /// Name of the tag.
    std::string name;

    /// Name of the branch that tracks commits to /tags/branch/
    std::string tag_branch;

    /// Committer.
    const Committer& committer;

    /// Time.
    time_t time;

    /// Log message.
    std::string log;

    Tag( const Committer& committer_, const std::string& name_, time_t time_, const std::string& log_ );
};

typedef unsigned short BranchId;

class Repository
{
    /// Remember what files we changed and how (deletes/modifications).
    std::string file_changes;

    /// Remember the copies of paths.
    ///
    /// We cannot use file_changes here, because this has to happen before the
    /// deletion of the source paths.
    std::string path_copies;

    /// Counter for the files.
    unsigned int mark;

    /// Regex for matching the fnames.
    regex_t regex_rule;

    /// Let's store to files.
    ///
    /// There can be a wrapping script that sets them up as named pipes that
    /// can feed the git fast-import(s).
    std::ofstream out;

    /// We have to remember our commits
    ///
    /// Index - commit number, content - branch id.
    BranchId* commits;

    /// Max number of revisions.
    unsigned int max_revs;

public:
    /// The regex_ is here to decide if the file belongs to this repository.
    Repository( const std::string& reponame_, const std::string& regex_, unsigned int max_revs_ );

    ~Repository();

    /// Does the file belong to this repository (based on the regex we got?)
    bool matches( const std::string& fname_ ) const;

    /// The file should be marked for deletion.
    void deleteFile( const std::string& fname_ );

    /// The file should be marked for addition/modification.
    std::ostream& modifyFile( const std::string& fname_, const char* mode_ );

    /// Commit all the changes we did.
    void commit( const Committer& committer_, const std::string& name_, unsigned int commit_id_, time_t time_, const std::string& log_, bool force_ = false );

    /// Create a branch.
    void createBranch( unsigned int from_, const std::string& from_branch_,
            const Committer& committer_, const std::string& name_, unsigned int commit_id_, time_t time_, const std::string& log_ );

    /// Create a tag (based on the 'tag tracking' branch).
    void createTag( const Tag& tag_ );

private:
    /// Find the most recent commit to the specified branch smaller than the reference one.
    unsigned int findCommit( unsigned int from_, const std::string& from_branch_ );
};

namespace Repositories
{
    /// Load the repositories layout from the config file.
    bool load( const char* fname_, unsigned int max_revs_, std::string& trunk_base_, std::string& trunk_, std::string& branches_, std::string& tags_ );

    /// Close all the repositories.
    void close();

    /// Get the right repository according to the filename.
    Repository& get( const std::string& fname_ );

    /// The file should be marked for deletion.
    inline void deleteFile( const std::string& fname_ ) { get( fname_ ).deleteFile( fname_ ); }

    /// The file should be marked for addition/modification.
    inline std::ostream& modifyFile( const std::string& fname_, const char* mode_ ) { return get( fname_ ).modifyFile( fname_, mode_ ); }

    /// Commit to the all repositories that have some changes.
    void commit( const Committer& committer_, const std::string& name_, unsigned int commit_id_, time_t time_, const std::string& log_ );

    /// Create a branch or a tag in all the repositories.
    void createBranchOrTag( bool is_branch_, unsigned int from_, const std::string& from_branch_,
            const Committer& committer_, const std::string& name_, unsigned int commit_id_, time_t time_, const std::string& log_ );

    /// Should the revision with this number be ignored?
    bool ignoreRevision( unsigned int commit_id_ );

    /// Should the tag with this name be ignored?
    bool ignoreTag( const std::string& name_ );
}

#endif // _REPOSITORY_HXX_
