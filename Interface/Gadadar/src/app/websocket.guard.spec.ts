import { TestBed } from '@angular/core/testing';
import { CanActivateFn } from '@angular/router';

import { websocketGuard } from './websocket.guard';

describe('websocketGuard', () => {
  const executeGuard: CanActivateFn = (...guardParameters) => 
      TestBed.runInInjectionContext(() => websocketGuard(...guardParameters));

  beforeEach(() => {
    TestBed.configureTestingModule({});
  });

  it('should be created', () => {
    expect(executeGuard).toBeTruthy();
  });
});
