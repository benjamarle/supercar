const rewireFlatStaticFolder = require('react-app-rewire-flat-static-folder');
 
/* config-overrides.js */
module.exports = function override(config, env) {
  config = rewireFlatStaticFolder(config, env);
  return config;
}