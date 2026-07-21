module.exports = {
  extends: ['@commitlint/config-conventional'],
  rules: {
    'type-enum': [2, 'always', [
      'feat',
      'fix',
      'docs',
      'refactor',
      'improve',
      'test',
      'chore',
      'revert',
    ]],
    'subject-case': [0],
    'subject-max-length': [1, 'always', 50],
    'body-max-line-length': [0],
    'header-max-length': [2, 'always', 80],
  },
};
