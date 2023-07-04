import { platformBrowserDynamic } from '@angular/platform-browser-dynamic';

import { AppRoutingModule } from './app/app.module';


platformBrowserDynamic().bootstrapModule(AppRoutingModule)
  .catch(err => console.error(err));
