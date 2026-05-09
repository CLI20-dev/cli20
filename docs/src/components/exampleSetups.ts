/** Emscripten FS setup callbacks for interactive playground examples.
 *
 * The Emscripten virtual filesystem root "/" always exists.
 * The current working directory defaults to "/", so relative paths
 * like "config.toml" resolve to "/config.toml".
 */

export function setupAdvancedFs(fs: any) {
  // --config requires an existing regular file.
  fs.writeFile('/config.toml', '');
  // /configs is a directory — passing it as --config shows is_regular_file failing.
  fs.mkdir('/configs');
}

export function setupValidationFs(fs: any) {
  // --output requires parent_exists. /tmp gives users a writable parent to try.
  fs.mkdir('/tmp');
}
